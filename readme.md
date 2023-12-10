
# simple_http
Это небольшой http-сервер для Колибри ОС позволяющий хостить статичные сайты и создавать модули, для динамической генерации отправляемых клиенту данных. 

Сервер отправляет содержимое файлов без сжатия в соответствии с заданной таблицей ассоциации MIME типа и расширения файла.
Если запрос от клиента имеет uri путь который соответствует модулю сервера, то сервер передаёт управление коду в этом модуле с передачей всех необходимых для функционирования данных.


## Configuration
Для настройки сервера применяется файл конфигурации в формате ini, где указываются следующие параметры:

В секции <CODE>MAIN</CODE>
 - <CODE>ip</CODE> - ip адрес сервера 
 - <CODE>port</CODE> - порт для подключения (по умолчанию 80)
 - <CODE>conn</CODE> - максимальное количество открытых соединений(по умолчанию 10)
 - <CODE>work_dir</CODE> - директория для размещения файлов, отправляемых сервером
 - <CODE>mime_file</CODE> - путь к файлу с таблицей сопоставлениея mime типов и расширений файлов (если не указан, то используется встроенная в сервер таблица сопоставления) 
 - <CODE>unit_dir</CODE> - директория расположения модулей сервера

 В секции <CODE>UNITS</CODE> может находиться множество параметров, имеющих вид <CODE>uri_path=file_name</CODE> где: 
 - <CODE>uri_path</CODE> - путь указываемый клиентом во время запроса
 - <CODE>file_name</CODE> - название/путь до файла модуля относительно <CODE>work_dir</CODE>



## API for units

К серверу можно подключить дополнительные модули в виде библиотек со специальными экспортируемыми функциями:

 - <CODE>uint32_t stdcall httpd_init(IMPORT_DATA* import)</CODE>

Эта функция необходима для передачи модулю необходимых данных, таких как функции работы с сетью, рабочие директории и тд.
Если инициализация модуля прошла успешно, функция возвращает 0.

 - <CODE>void stdcall httpd_server(CONNECT_DATA* request_data)</CODE>
Эта функция вызывается при получении запроса с uri путём указанном в файле конфигурации для этого модуля. Сервер передаёт в функцию структуру соединения, в которой находятся данные запроса(заголовки, параметры, http метод и версия). На основе этих данных функция может генерировать необходимый ответ. 

 
## Bugs 

 - В ходе тестов был обнаружена ошибка отправки "больших" файлов. Это баг сетевого стека.
 - При длительной работе сервер может начать "подзависать" или перестать отвечать на сообщения. Это баг сетевого стека.
