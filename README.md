Сербаев Вадим, СКБ 172. Отчет к заданию по дисциплине «Современные технологии программирования и обработки информации».

# Программная реализация Telnet-подобного сервера

## Цель работы

Цель работы заключается в программной реализации асинхронного Telnet-подобного сервера, написанного на языке C++ с использованием библиотеки Boost.Asio и обратных вызовов (callbacks) без сопроцедур (boost::asio::spawn, ca::spawn, boost::asio::co_spawn).

Программный код является оригинальным и написал с нуля в среде Visual Studio 2019. Стандарт языка - ISO C++ 17.

## О Telnel

TELNET — это средство связи, сетевой протокол, которое устанавливает транспортное соединение между терминальными устройствами, клиентом и сервером, поддерживающими этот стандарт соединения.
Telnet можно использовать для:
- подключения к удалённым компьютерам;
- проверки порта на наличие доступа;

## О библиотеке Boost.Asio

Boost.Asio - это кросс-платформенная С++ библиотека для программирования сетей и низкоуровневых программ ввода/вывода. Asio был принят в Boost в 2005 году. Первоначально автором является Кристофер Кохлхофф (Christopher M. Kohlhoff).

Boost.Asio является заголовочной библиотекой.

В своей реализации я использовал Boost версии 1.77. Установка библиотеки была произведена при помощи встроенного диспетчера пакетов NuGet:

![Альтернативный текст](https://sun9-62.userapi.com/impg/E6DXinQo0Cd8WCB6rhMep0vlFyFlsll-M3qn7g/IoKaUTqVCPQ.jpg?size=747x384&quality=96&sign=61552e0243e7f2e4d6d61759e36ef3dd&type=album)

![Альтернативный текст](https://sun9-57.userapi.com/impg/CKSpTB-MMSy7Y0exkpeTgvi_W1M8ZCMygflgAw/pBF6Rzzqy5M.jpg?size=2148x478&quality=96&sign=f07011827c478b64260b12a0edda875c&type=album)

## Различия синхронного и асинхронного программирования

В синхронном программировании операции выполняются в последовательном порядке, такие как чтение из сокета и запись в сокет. Каждая из операций является блокирующей, поэтому, чтобы не прерывать основную программу, в то время как происходит чтение или запись в сокет, обычно приходится создавать один или несколько потоков, которые имеют дело с сокетами ввода/вывода. Таким образом, синхронные сервер/клиенты, как правило, многопоточны.

В случае асинхронного программирования не обязательно иметь более одного потока.
Асинхронные программы управляются событиями.

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

    - получение предусмотренной команды и корректное заполнение аргументов => вызывается код её выполнения и формирование ответа клиенту. 
        
     - получение предусмотренной команды и некорректное заполнение аргументов => формирование ответа клиенту о неправильном заполнении аргументов.
        
     - получение непредусмотренной команды => формирование соответствующего ответа об ошибке.

- Шаг 4. Процесс отправки ответа клиенту.

- Шаг 5. Происходит считывание строки до символа переноса строки (\n) (шаг № 1).


## Демонстрация работы программы

1. После успешного запуска программы (окно справа) пытаюсь подключиться к заданному порту 23, поддерживающему TCP протокол: 

![Альтернативный текст](https://sun9-7.userapi.com/impg/Y_XfffLKhB4hPJc2dl2oEtDJAFYNp7DJuioeHg/kDAzt7UHaok.jpg?size=1992x532&quality=96&sign=7a73b48ee56873043e6c4530f2544fca&type=album)

2. Демонстрация успешного подключения клиента к серверу:

![Альтернативный текст](https://sun9-48.userapi.com/impg/Sm2bugfNUfbKMMYduwthzaebiV2_Gg9TGYvtfA/HlkR1j5Z8aQ.jpg?size=1984x522&quality=96&sign=28616a6e16e3f9dbdfaa861e15693bb6&type=album)

3. Отправлю любой текст, кроме пробелов, на сервер и получу инструкцию по работе с псевдотерминалом:

![Альтернативный текст](https://sun9-72.userapi.com/impg/PX33Tjixrdycib4TK5yW3rHg2_FTQ6rixbF8kg/PfS9FsOds8c.jpg?size=1979x519&quality=96&sign=1d8ddaa08df7e68efad1c06a9c578e07&type=album)

4. Выполню каждую команду:

![Альтернативный текст](https://sun9-67.userapi.com/impg/jUdqaXL7miqton615gxuQhF9GmXsUnMRTFU3Jw/Q_8_PbqP-Rk.jpg?size=918x600&quality=96&sign=2aaf99b6a5b542d116c64a00b35af9f5&type=album)

Как видно из окна терминала, программа успешно выполняет предусмотренные команды.

5. Сервер успешно обрабатывает команды нескольких клиентов:

![Альтернативный текст](https://sun9-39.userapi.com/impg/tD443rnp6sjHJOoDBYrhyivYh5taxyuwi9Kqjg/NI7jkKuAaGA.jpg?size=1963x1022&quality=96&sign=044b64b3a2710feeca124b7615b70d5d&type=album)

Также сервер реагирует соответствующими сообщениями о неправильном вводе команды / аргументов / возникающих ошибках.
