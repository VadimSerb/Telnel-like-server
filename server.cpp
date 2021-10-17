#include <iostream>
#include <filesystem>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <optional>
#include <queue>
#include <unordered_set>

namespace io = boost::asio;
namespace ip = io::ip;
using tcp = ip::tcp;
using error_code = boost::system::error_code;
using namespace std::placeholders;

using string_group = std::vector<std::string>;

struct dispatcher_entry
{
    std::string const description;
    std::size_t const args;
    std::function<std::string(string_group const&)> const handler;
};

using dispatcher_type = std::map<std::string, dispatcher_entry>;

string_group split(std::string const& string)
{
    using separator = boost::char_separator<char>;
    using tokenizer = boost::tokenizer<separator>;

    string_group group;

    for (auto&& str : tokenizer(string, separator(" ")))
    {
        group.emplace_back(str);
    }

    return group;
}

class session : public std::enable_shared_from_this<session>
{
public:

    session(tcp::socket&& socket, dispatcher_type const& dispatcher)
        : socket(std::move(socket))
        , dispatcher(dispatcher)
    {
    }

    void start()
    {
        write("Welcome to my very own terminal server!");
        write();
        read();
    }

private:

    void write(std::string const& string)
    {
        outgoing.push(string + "\r\n> ");
    }

    void read()
    {
        io::async_read_until(socket, incoming, "\n", std::bind(&session::on_read, shared_from_this(), _1, _2));
    }

    void on_read(error_code error, std::size_t bytes_transferred)
    {
        if (!error)
        {
            std::istream stream(&incoming);
            std::string line;
            std::getline(stream, line);
            incoming.consume(bytes_transferred);
            boost::algorithm::trim(line);
            if (!line.empty())
            {
                dispatch(line);
            }
            read();
        }
    }

    void write()
    {
        io::async_write(socket, io::buffer(outgoing.front()), std::bind(&session::on_write, shared_from_this(), _1, _2));
    }

    void on_write(error_code error, std::size_t bytes_transferred)
    {
        if (!error)
        {
            outgoing.pop();

            if (!outgoing.empty())
            {
                write();
            }
        }
    }

    void dispatch(std::string const& line)
    {
        auto command = parse(line);
        std::stringstream response;

        if (auto it = dispatcher.find(command.first); it != dispatcher.cend())
        {
            auto const& entry = it->second;

            if (entry.args == command.second.size())
            {
                try
                {
                    response << entry.handler(command.second);
                }
                catch (std::exception const& e)
                {
                    response << "An error has occurred: " << e.what();
                }
            }
            else
            {
                response << "Wrong arguments count. Expected: " << entry.args << ", provided: " << command.second.size();
            }
        }
        else
        {
            response << "Command not found. Available commands:\n\r";

            for (auto const& [cmd, entry] : dispatcher)
            {
                response << "- " << cmd << ": " << entry.description << "\n\r";
            }
        }

        write(response.str());

        if (outgoing.size() == 1)
        {
            write();
        }
    }

    static std::pair<std::string, string_group> parse(std::string const& string)
    {
        string_group args = split(string);
        auto name = std::move(args[0]);
        args.erase(args.begin());
        return { name, args };
    }

    tcp::socket socket;
    dispatcher_type const& dispatcher;
    io::streambuf incoming;
    std::queue<std::string> outgoing;
};

class server
{
public:

    server(io::io_context& io_context, std::uint16_t port)
        : io_context(io_context)
        , acceptor(io_context, tcp::endpoint(tcp::v4(), port))
    {
        accept();
    }

    template <typename F>
    void attach(std::string const& pattern, std::string const& description, F&& f)
    {
        auto cmd = split(pattern);

        dispatcher.emplace(cmd[0], dispatcher_entry
            {
                description,
                cmd.size() - 1,
                std::move(f)
            });
    }

private:

    void accept()
    {
        socket.emplace(io_context);

        acceptor.async_accept(*socket, [&](error_code error)
            {
                std::make_shared<session>(std::move(*socket), dispatcher)->start();
                accept();
            });
    }

    io::io_context& io_context;
    tcp::acceptor acceptor;
    std::optional<tcp::socket> socket;
    dispatcher_type dispatcher;
};

int main(int argc, char* argv[])
{

    io::io_context io_context;
    server srv(io_context, 15001);

    srv.attach("date", "Print current date and time", [](string_group const& args)
        {
            auto now = boost::posix_time::second_clock::local_time();
            return to_simple_string(now);
        });

    srv.attach("pwd", "Print working directory", [](string_group const& args)
        {
            return std::filesystem::current_path().string();
        });

    srv.attach("dir", "Print working directory's content", [](string_group const& args)
        {
            std::stringstream result;

            std::copy
            (
                std::filesystem::directory_iterator(std::filesystem::current_path()),
                std::filesystem::directory_iterator(),
                std::ostream_iterator<std::filesystem::directory_entry>(result, "\n\r")
            );

            return result.str();
        });

    srv.attach("cd %", "Change current working directory", [](string_group const& args)
        {
            std::filesystem::current_path(args[0]);
            return "Working directory: " + std::filesystem::current_path().string();
        });

    srv.attach("mul % %", "Multiply two numbers", [](string_group const& args)
        {
            double a = boost::lexical_cast<double>(args[0]);
            double b = boost::lexical_cast<double>(args[1]);
            return std::to_string(a * b);
        });

    io_context.run();

    return 0;
}