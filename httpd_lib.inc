
align   4
day:
                dd 'Mon,'
                dd 'Tue,'
                dd 'Wed,'
                dd 'Thu,'
                dd 'Fri,'
                dd 'Sat,'
                dd 'Sun,'
.count = ($ - day) / 4

align   4
months:
                dd 'Jan '
                dd 'Feb '
                dd 'Mar '
                dd 'Apr '
                dd 'May '
                dd 'Jun '
                dd 'Jul '
                dd 'Aug '
                dd 'Sep '
                dd 'Oct '
                dd 'Nov '
                dd 'Dec '
.count = ($ - months) / 4 ; count item in this array

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




serv_header:
        .Accept_Ranges  db 'Accept-Ranges: bytes',13,10
        .connection     db 'Connection: close',0

http_method:
        .get:   db 'GET '
        .head:  db 'HEAD'
        .post:  db 'POST'
        .put:   db 'PUT '
        .patch  db 'PATCH'

;error_404:
;        db '<html>'
;        db '<title>Error 404</title>'
;        db '<center>'
;        db             'Error 404 <br>'
;        db 'The server could not find the requested page.<br><br>'
;        db '</center>'
;        db '</html>',0

http_err_response:
                db 'HTTP/1.1 '
.code = $ - http_err_response
                db '000 ',13, 10
                db 'Server: simple-httpd/0.0.1', 13, 10
                db 'Date: '
.date = $ - http_err_response
                db 'Sun, 30 Oct 2022 09:29:13 GMT',13, 10
                db 'Content-length: 0', 13, 10
                db 'Content-type: text/plain', 13, 10;
                db 'Connection: close', 13, 10
                db 13, 10
.size = $ - http_err_response


base_response:
label response at 0
                db 'HTTP/1.0 '
.code:          db '000 ',13, 10
                db 'Server: httpd(kolibri os)/0.0.1', 13, 10
                db 'Cache-Control: no-cache', 13, 10
                db 'Content-Encoding: '
.content_encod: db 'identity', 13, 10
                db 'Date: '
.date:          db 'Sun, 30 Oct 2022 09:29:13 GMT',13, 10
                db 'Content-length: '
.content_len:   db '                      ', 13, 10
                db 'Content-type: '
.content_type:  db '                                                                ', 13, 10;
                  ;'text/html; charset=utf-8'
.end_headers:         ;нужно, когда базового заголовка не хватает
.connection     db 'Connection: close', 13, 10
                db 13, 10
.body:          ; с этого оффсета уже писать данные

; min HTTP request size
; "GET / HTTP/1.1" - 18 byte
min_http_size   = 18
