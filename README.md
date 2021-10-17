Сербаев Вадим, СКБ 172. Отчет к заданию по дисциплине «Современные технологии программирования и обработки информации».

# Программная реализация Telnet-подобного сервера

## Цель работы

Цель работы заключается в программной реализации Telnet-подобного сервера, написанного на языке C++ с использованием библиотеки Boost.Asio и обратных вызовов (callbacks) без сопроцедур (boost::asio::spawn, ca::spawn, boost::asio::co_spawn).

Программный код является оригинальным и написал с нуля.

## О Telnel

TELNET — это средство связи, сетевой протокол, которое устанавливает транспортное соединение между терминальными устройствами, клиентом и сервером, поддерживающими этот стандарт соединения.
Telnet можно использовать для:
- подключения к удалённым компьютерам;
- проверки порта на наличие доступа;


## Логика работы программы

Программа представляет собой терминальный сервер, к которому можно подключиться при помощи Telnet. После успешного подключения можно отправлять и выполнять предусмотренные команды на стороне сервера. После каждого получения прописанной команды сервер отправляет клиенту соответсвующий ответ о её выполнении. В случае получения непредусмотренной команды сервер вышлет соответствующую ошибку.

Начало работы программы - после успешного подключения сервер приветствует клиента текстовым сообщением, после чего предлагает ввести команду.

Программу можно логически разделить на серверную и сессионную части. Рассмотрим алгоритм работы каждой части.

#### Алгоритм сервера

Серверная часть работает следующим циклическим образом:

- Шаг 1. Принимает входящее соединение на заданном порту.

- Шаг 2. Для принятого соединения создаёт объект сеанса.

- Шаг 3. Принимает входящее соединение на заданном порту (шаг № 1).

#### Алгоритм сессии

Сессионная часть работает следующим циклическим образом:

- Шаг 1. Происходит считывание строки до символа переноса строки (\n).

- Шаг 2. Происходит парсинг строки, вычленение команды и аргумента(-ов).

- Шаг 3. Возможны 3 варианта: 
- получение предусмотренной команды => вызывается код её выполнения и формирование ответа клиенту. 

- Шаг 4. Процесс отправки ответа клиенту.

- Шаг 5. Происходит считывание строки до символа переноса строки (\n) (шаг № 1).

## Классы, функции



## Демонстрация работы программы

