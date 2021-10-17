#include <iostream>
#include <filesystem>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <optional>
#include <queue> // библиотека для стековой очереди
#include <unordered_set>

//сокращения
namespace io = boost::asio;
namespace ip = io::ip;
using tcp = ip::tcp;
using error_code = boost::system::error_code;

using string_group = std::vector<std::string>; //вектор строк; используется как вектор подстрок строки при разделении начальной строки

using namespace std::placeholders; //пространство имен для использования связвателя-функции bind для задания порядка свободных аргументов
                                   //смотри session -> read, session -> write


struct dispatcher_entry //структура для хранения информации о команде, которую нужно выполнить
{
    std::string const description; // описание команды
    std::size_t const args; // число аргументов команды
    std::function<std::string(string_group const&)> const handler; //обработчик команды
};

using dispatcher_type = std::map<std::string, dispatcher_entry>; //контенер, хранящий информацию о командах, предусмотренных для выполнения
                                                                 //ключ - строка-название команды, значение - структура, хранящая информацию по каждой команде

string_group split(std::string const& string) // вспомогательная функция для разделения строки на вектор подстрок этой строки; возвращает вектор строк
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
        write("Welcome to my Telnet-like terminal server!"); //приветствуем клиента
        write(); //
        read(); //
    }

private:

    void write(std::string const& string) //функция для печати приветствия и ответов
    {
        outgoing.push(string + "\r\n> "); // в порядке очереди посылаем ответ клиенту
        // Пояснение - под Windows: (в Unix достаточно \n)
        // \n - совершает переход на одну строку, но не сохраняет позицию
        // \r - перемещает позицию в начало (чтобы следующий символ был вначале строки, а не в конце)
    }

    void read() //считывает получаемый текст при помощи обратного вызова async_read_until до символа переноса строки
    {
        //async_read_until функция асинхронного чтения; читает данные из сокета в streambuf до символа переноса строки; обнаружив символ вызывается обработчик завершения
        io::async_read_until(socket, incoming, "\n", std::bind(&session::on_read, shared_from_this(), _1, _2)); 
        //после считывания происходит анализ через функцию on_read
    }

    void on_read(error_code error, std::size_t bytes_transferred)
    {
        if (!error)
        {
            std::istream stream(&incoming);
            std::string line;
            std::getline(stream, line); //записываем из условного потокового ввода текст-содержимое в line
            incoming.consume(bytes_transferred); // очищаем incoming
            boost::algorithm::trim(line); //зачищаем нули по обеим сторонам
            if (!line.empty()) //проверяем строку, не является ли она пустой
            {
                dispatch(line);
            }
            read(); 
        }
    }

    void write() 
    {
        //async_write функция асинхронной записи в поток
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

    void dispatch(std::string const& line) //определяет тип команды, которую нужно выполнить и формируем ответ
    {
        auto command = parse(line); //формируем пару команда-аргументы
        std::stringstream response; //формируем ответный поток

        if (auto it = dispatcher.find(command.first); it != dispatcher.cend()) //ищем команду среди всех ключей
        {
            auto const& entry = it->second;

            if (entry.args == command.second.size()) //сравниваем число аргументов команды
            {
                try
                {
                    response << entry.handler(command.second); //записываем в ответ результат выполнения команды
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
        else //в случае, когда команда не найдена среди ключей, формируем в ответ все доступные команды
        {
            response << "Command not found. Available commands:\n\r";

            for (auto const& [cmd, entry] : dispatcher)
            {
                response << "- " << cmd << ": " << entry.description << "\n\r";
            }
        }

        write(response.str()); //помещаем ответ в исходящую очередь

        if (outgoing.size() == 1) //проверяем, была ли очередь пустой до этого; если да - инициируем доставку сообщения обратно клиенту
        {
            write();
        }
    }

    static std::pair<std::string, string_group> parse(std::string const& string) //функция анализа введенной клиентом строки
    {
        string_group args = split(string); //разделяем строку на подстроки -> вектор подстрок
        auto name = std::move(args[0]); //полученная команда
        args.erase(args.begin()); //аргументы команды
        return { name, args }; //возвращает пару: имя команды, вектор аргументов
    }

    tcp::socket socket; //TCP-сокет
    dispatcher_type const& dispatcher; //указатель на map-контейнер обработчиков команд терминала
    io::streambuf incoming; // streambuf для входящих данных, работает как std::streambuf
    std::queue<std::string> outgoing; //очередь исходящих сообщений
};

class server
{
public:

    server(io::io_context& io_context, std::uint16_t port)
        : io_context(io_context)
        , acceptor(io_context, tcp::endpoint(tcp::v4(), port)) //объект, принимающий все входящие TCP соединения на заданном порту
                                                               //иначе говоря, acceptor прослушивает все соединения на указанном порту
    {
        accept(); //функция приема входящего соединения
    }

    template <typename F> //шаблон исполняемой команды

    void attach(std::string const& pattern, std::string const& description, F&& f) // функция, в которую передается шаблон команды, описание команды и ее обработчик.
                                                                                   // Шаблон команды состоит из имени и необязательных заполнителей параметров.
        //Например, pattern-ом будет: cd %, dir, date, pwd
    {
        auto cmd = split(pattern); //делаем строковый string_group массив, содержащий подстроки pattern

        dispatcher.emplace(cmd[0], dispatcher_entry //заполняем map-контейнер dispatcher
            {
                description, //собственно, описание команды
                cmd.size() - 1, //число аргументов команды; т.к. не учитываем саму команду => - 1
                std::move(f) //функция-обработчик команды (через перевод в rvalue)
                
            });
    }

private:

    void accept() //функция приема входящего соединения
    {
        socket.emplace(io_context); // заполнение сокета

        acceptor.async_accept(*socket, [&](error_code error)
            {
                std::make_shared<session>(std::move(*socket), dispatcher)->start();
                accept();
            });
        //сервер ожидает нового входящего соединения, и когда он его получает, он создает объект сеанса и перемещает сокет, связанный с принятым соединением, внутрь сеанса.
        //помимо сокета, внутрь сеанса отправляется map-контейнер, содержащий всю информацию для выполнения определенных мною команд
        //после этого сервер начинает ждать очередного входящего соединения.
    }

    io::io_context& io_context; // ссылка на объект io_context из main
    tcp::acceptor acceptor; // объект, принимающий входящие соединения
    std::optional<tcp::socket> socket; // ТСР сокет
    dispatcher_type dispatcher; // формирующийся функцией attach map-контейнер обработчиков команд терминала
                                // фактически, набор инструкций при получении той или иной команды
};

int main(int argc, char* argv[])
{

    io::io_context io_context; //создаем объект класса io_context
    //io_context по сути обеспечивает основную функцию ввода - вывода пользовательского объекта асинхронного ввода - вывода и является неотъемлимой часть при асинхронном взаимодействии
    server srv(io_context, 15001); //инициализируем объект-сервер: второй аргумент - порт

    //описываем серверные команды, доступные для выполнения
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

    io_context.run(); //своего рода, функция цикла событий, которая управляет всеми операциями ввода-вывода.

    return 0;
}