
;align   4
;day:
;                dd 'Mon,'
;                dd 'Tue,'
;                dd 'Wed,'
;                dd 'Thu,'
;                dd 'Fri,'
;                dd 'Sat,'
;                dd 'Sun,'
;.count = ($ - day) / 4
;
;align   4
;months:
;                dd 'Jan '
;                dd 'Feb '
;                dd 'Mar '
;                dd 'Apr '
;                dd 'May '
;                dd 'Jun '
;                dd 'Jul '
;                dd 'Aug '
;                dd 'Sep '
;                dd 'Oct '
;                dd 'Nov '
;                dd 'Dec '
;.count = ($ - months) / 4 ; count item in this array

;       HTTP-date    = rfc1123-date | rfc850-date | asctime-date
;       rfc1123-date = wkday "," SP date1 SP time SP "GMT"
;       rfc850-date  = weekday "," SP date2 SP time SP "GMT"
;       asctime-date = wkday SP date3 SP time SP 4DIGIT
;       date1        = 2DIGIT SP month SP 4DIGIT
;                      ; day month year (e.g., 02 Jun 1982)
;       date2        = 2DIGIT "-" month "-" 2DIGIT
;                      ; day-month-year (e.g., 02-Jun-82)
;       date3        = month SP ( 2DIGIT | ( SP 1DIGIT ))
;                      ; month day (e.g., Jun  2)
;       time         = 2DIGIT ":" 2DIGIT ":" 2DIGIT
;                      ; 00:00:00 - 23:59:59
;       wkday        = "Mon" | "Tue" | "Wed"
;                    | "Thu" | "Fri" | "Sat" | "Sun"
;       weekday      = "Monday" | "Tuesday" | "Wednesday"
;                    | "Thursday" | "Friday" | "Saturday" | "Sunday"
;       month        = "Jan" | "Feb" | "Mar" | "Apr"
;                    | "May" | "Jun" | "Jul" | "Aug"
;                    | "Sep" | "Oct" | "Nov" | "Dec"




;serv_header:
;        .Accept_Ranges  db 'Accept-Ranges: bytes',13,10

;http_err_response:
;                db 'HTTP/1.1 '
;.code = $ - http_err_response
;                db '000 ',13, 10
;                db 'Server: simple-httpd/0.0.1', 13, 10  ; httpd(kolibri os)/0.0.1, 13, 10
;                db 'Date: '
;.date = $ - http_err_response
;                db 'Sun, 30 Oct 2022 09:29:13 GMT',13, 10
;                db 'Content-length: 0', 13, 10
;                db 'Content-type: text/plain', 13, 10;
;                db 'Connection: close', 13, 10
;                db 13, 10
;.size = $ - http_err_response

default_http_version:
                db 'HTTP/1.1 '
.length = $ - default_http_version

default_http_connection:
                db 'Connection: '
.value =  $ - default_http_connection
                db 'close     ', 13, 10 ; or keep-alive
.length = $ - default_http_connection

default_http_cont_encod:
                db 'Content-Encoding: identity', 13, 10
.length = $ - default_http_cont_encod

default_http_cont_type:
                db 'Content-type: '
.value =  $ - default_http_cont_type
                db 'text/html                                                       ', 13, 10;
.length = $ - default_http_cont_type

default_http_cache_ctl:
                db 'Cache-Control: no-cache', 13, 10
.length = $ - default_http_cache_ctl

default_http_cont_len:
                db 'Content-length: '
                db '0000000000000000000000', 13, 10
.length = $ - default_http_cont_len

default_http_header_server:
                db 'Server: simple-httpd/0.3.0', 13, 10
.length = $ - default_http_header_server

http_header_transfer_chunked:
                db 'Transfer-Encoding: chunked', 13, 10
.length = $ - http_header_transfer_chunked

;default_http_date_header:
;                db 'Date: '
;.date:          db 'Sun, 30 Oct 2022 09:29:13 GMT',13, 10
;.length = $ - default_http_date_header


http_response_err_501:
                db 'HTTP/1.1 '
                db '501 ',13, 10
                db 'Error parsing your request message. The version is not supported or another error.'
                db 'Server: simple-httpd/0.3.0', 13, 10
                db 'Connection: close', 13, 10
                db 13, 10
.size = $ - http_response_err_501

http_response_err_404:
                db 'HTTP/1.1 '
                db '404 ',13, 10
                db 'Server: simple-httpd/0.3.0', 13, 10
                db 'Content-length: 45', 13, 10
                db 'Content-type: text/plain', 13, 10;
                db 'Connection: close', 13, 10
                db 13, 10
                db 'The server could not find the requested page.'
.size = $ - http_response_err_404

http_response_options:
                db 'HTTP/1.1 '
                db '204 ',13, 10
                db 'Server: simple-httpd/0.3.0', 13, 10
                db 'Allow: OPTIONS, GET, POST', 13, 10
                db 'Connection: close', 13, 10
                db 13, 10
.size = $ - http_response_options


; min HTTP request size
; "GET / HTTP/1.1" - 18 byte
min_http_size   = 18

MIME_FILE_FORMAT:
.html:          db 5,'.html', 'text/html',0
.css:           db 4,'.css',  'text/css ',0
.js:            db 3,'.js',   'text/javascript',0
.txt:           db 4,'.txt',  'text/plain',0
.pdf:           db 4,'.pdf',  'application/pdf',0
.json:          db 5,'.json', 'application/json',0

.png:           db 4,'.png',  'image/png',0
.mp3:           db 4,'.mp3',  'audio/mpeg',0
.mp4:           db 4,'.mp4',  'video/mp4',0
.other:         dd 0
                db 'application/octet-stream',0 ; for unknow file - all file :)

STD_MIME_TYPE_ARR:
        dd      MIME_FILE_FORMAT.html, \
                MIME_FILE_FORMAT.css, \
                MIME_FILE_FORMAT.js, \
                MIME_FILE_FORMAT.txt, \
                MIME_FILE_FORMAT.pdf, \
                MIME_FILE_FORMAT.json, \
                MIME_FILE_FORMAT.png, \
                MIME_FILE_FORMAT.mp3, \
                MIME_FILE_FORMAT.mp4, \
                MIME_FILE_FORMAT.other ; terminate 

_DIV_10_:       dd 10
_DIV_100_:      dd 100
_DIV_1000_:     dd 1000

hex_chars:
        db '0123456789ABCDEF'
