
# simple_httpd
Это небольшой http-сервер для Колибри ОС позволяющий хостить статичные сайты и создавать модули, для динамической генерации отправляемых клиенту данных. 

Сервер отправляет содержимое файлов без сжатия в соответствии с заданной таблицей ассоциации MIME типа и расширения файла.
Если запрос от клиента имеет uri путь который соответствует модулю сервера, то сервер передаёт управление коду в этом модуле с передачей всех необходимых для функционирования данных.

## install 
Для установки сервера на диск скопируйте файлы из директории bin данного репозитория. В этой директории находятся слудеющие файлы:
 - `httpd` - исполняемый файл сервера
 - `mime_types.bin` - файл с расширенной таблицей ассоцияции MIME типа с расширением файла
 - `httpd.ini` - файл конфигурации сервера

и директории:
 - `modules` - Директория в которой хранятся некоторые примеры модулей, для демонстрации возможностей сервера
 - `server_data` - Директория для размещения статичных данных сервера. Изначально в ней находится только документация по использованию сервером.

Готовый файл конфигурации уже настроен для использование и ожидает, что всё содержимое директории bin репозитория будет размешено по пути `/usbhd0/3/`. По этому для установки достаточно скопировать содержимое в корень третьего раздела usb диска и запустить файл httpd .

Подробная настройка сервера описана в документации, расположенной в директории doc этого репозитория.  

## TODO
### Tasks on version 0.3.0
 - Добавить модуль демонстрации websockets
 - Добавить демонстрационный модуль на Си
 - Добавить демонстрационный модуль на FPC
 - Добавить модуль тестовой авторизации(base64 code in header)
 - Добавить модуль генерации более сложного контента
   (create json object with data of CSV table)
 - Добавить поддержку TLS шифрования с использованием MbedTLS

## Bugs 
 - В ходе тестов был обнаружена ошибка отправки "больших" файлов. Это баг сетевого стека;
 - При длительной работе сервер может начать "подзависать" или перестать отвечать на сообщения. Это баг сетевого стека.
