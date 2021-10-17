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

Программа представляет собой терминальный сервер, к которому можно подключиться при помощи Telnet. После успешного подключения можно отправлять и выполнять предусмотренные команды на стороне сервера. После каждого получения прописанной команды сервер отправляет клиенту соответсвующий ответ о выполнении команды. В случае получения непредусмотренной команды сервер отправит соответствующую ошибку.

Начало работы программы - после успешного подключения сервер приветствует клиента текстовым сообщением, после чего предлагает ввести команду.

Программу можно логически разделить на серверную и сессионную части. Рассмотрим алгоритм работы каждой части.

#### Алгоритм сервера

#### Алгоритм сессии

## Классы, функции



## Демонстрация работы программы

