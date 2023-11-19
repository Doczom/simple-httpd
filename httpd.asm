;*****************************************************************************;
;      Copyright (C) 2023, Mikhail Frolov aka Doczom . All rights reserved.   ;
;            Distributed under terms of the 3-Clause BSD License.             ;
;                                                                             ;
;                 httpd - Simple http server for Kolibri OS.                  ;
;                                                                             ;
;                      Version 0.0.1, 12 November 2023                        ;
;                                                                             ;
;*****************************************************************************;
;include "macros.inc"
include 'D:\kos\programs\macros.inc'
;include 'D:\kos\programs\network.inc'
  use32
  org    0
  db     'MENUET01'
  dd     1, START, I_END, MEM, STACKTOP, PATH, 0
;KOS_APP_START

include 'sys_func.inc'
include 'settings.inc'

;CODE
START:
        mcall   68, 11  ; init heap
        mcall   40, EVM_STACK ;set event bitmap

        mov     ecx, PATH
        cmp     byte[ecx],0
        jnz     @f
        mov     ecx, default_ini_path
@@:
        ; get settings
        call    load_settings ; ecx -> string to config file
        test    eax, eax
        jnz     .err_settings

        ;init server socket
        push    dword SO_NONBLOCK ; IPPROTO_TCP
        push    dword SOCK_STREAM
        push    dword AF_INET4
        call    netfunc_socket; AF_INET4, SOCK_STREAM, SO_NONBLOCK ; we dont want to block on accept
        cmp     eax, -1
        je      .sock_err
        mov     [srv_socket], eax

        push    srv_sockaddr.length
        push    dword srv_sockaddr
        push    dword[srv_socket]
        call    netfunc_bind; [srv_socket], srv_sockaddr, srv_sockaddr.length
        cmp     eax, -1
        je      .bind_err

        ; listen()
        push    dword[srv_backlog]
        push    dword[srv_socket]
        call    netfunc_listen; [srv_socket], [srv_backlog]
        cmp     eax, -1
        jz      .listen_err

.mainloop:
        mcall   23, 100 ; get event to network stack 
        test    eax, eax
        jz      .mainloop

        push    dword thread_connect
        call    CreateThread ; not save PID
        jmp     .mainloop

.listen_err:
.bind_err:
        push    dword[srv_socket]
        call    netfunc_close; [srv_socket]

.err_settings:
.sock_err:
        mcall   -1



thread_connect:
        sub     esp, sizeof.CONNECT_DATA
        mcall   40, EVM_STACK ; set event bitmap - network event 

        ; ожидание подключения Accept, sockaddr находится на вершине стека нового потока
        ;lea     edx, [esp + CONNECT_DATA.sockaddr] ; new sockaddr
        ;push    dword 16 ; 16 byte - sockaddr length
        ;push    edx
        push    srv_sockaddr.length
        push    dword srv_sockaddr

        push    dword[srv_socket]
        call    netfunc_accept

        cmp     eax, -1
        jz      .err_accept
        
        mov     [esp + CONNECT_DATA.socket], eax
        mov     ebp, esp

        ; соединение установленно, теперь нужно выделить буфер(тоже на 16 кб наверное), и
        ; прочитать в этот буфер из сокета, когда прочтём ноль(или больше 4 кб), тогда
        ; выходим из цикла и анализируем заголовки и стартовую стоку.
        mov     esi, 0x8000
        push    esi
        call    Alloc ;alloc memory 32 kib
        test    eax, eax
        jz      .err_alloc

        mov     [esp + CONNECT_DATA.buffer_request], eax
        mov     edx, eax
        mov     esi, esp ; SAVE CONNECT_DATA
        mov     dword[esi + CONNECT_DATA.request_size], 0
@@:
        ;read data from socket
        push    dword 0 ;flags
        mov     eax, [esi + CONNECT_DATA.request_size]
        push    dword 0x8000
        sub     [esp], eax
        push    dword[esi + CONNECT_DATA.buffer_request]
        add     [esp], eax
        push    dword[esi + CONNECT_DATA.socket]
        call    netfunc_recv
        
        cmp     eax, -1
        jz      .err_recv_sock

        test    eax, eax
        jz      @f
        add     [esi + CONNECT_DATA.request_size], eax
 ;       cmp     [esi + CONNECT_DATA.request_size], 0x8000 ; check end buffer
 ;       jb      @b
@@:
        ; после получения всего запроса(более или менее всего) выделяем озу для 
        ; ассоциативного массива заголовков и аргументов запроса
        ; 8*50 + 8*100
        ;  esp        .. esp + 1024 -> for http headers
        ;  esp + 1024 .. esp + 2048 -> for URI args
        sub     esp, 2048 

        ; parse http message
        mov     ecx, [esi + CONNECT_DATA.buffer_request] 
        call    parse_http_query ; ecx - buffer 
        test    eax, eax
        jz      .err_parse
        ; вызов нужной функции из списка моделей
        
        ; TODO
        

        ; if not found units, call file_server
        call    file_server ; esi - struct 
        ;TEST SERVER, DELETE ON RELISE

        ; end work thread
        jmp     .end_work
        

.err_parse:
        call    file_server.err_http_501
.end_work:
        add     esp, 2048
        ; free OUT buffer
        cmp     dword[esp + CONNECT_DATA.buffer_response], 0
        jz      .err_recv_sock
        push    dword[esp + CONNECT_DATA.buffer_response]
        call    Free
.err_recv_sock:
        ; free IN buffer
        cmp     dword[esp + CONNECT_DATA.buffer_request], 0
        jz      .err_alloc
        push    dword[esp + CONNECT_DATA.buffer_request]
        call    Free
.err_alloc:
        push    dword[esp + CONNECT_DATA.socket]
        call    netfunc_close
.err_accept:
        lea     ecx,[esp + sizeof.CONNECT_DATA - 0x4000] ; get pointer to alloc memory
        mcall   68, 13 ; free
        mcall   -1     ; close thread

include 'parser.inc'

include 'file_server.inc'
; DATA AND FUNCTION
include 'httpd_lib.inc'

default_ini_path: db 'httpd.ini',0
I_END:
;DATA

; DATA

;UDATA

srv_backlog:    rd 1 ; максимум одновременных подключений подключений

srv_socket:     rd 1

srv_sockaddr:
                dw AF_INET4
  .port         dw 0
  .ip           dd 0
                rb 8
  .length       = $ - srv_sockaddr

GLOBAL_DATA:
        .units          rd 1 ; указатель на ассоциативный массив пути и указателя на функцию либы(см ниж)
        .unit_count     rd 1 ; количество записей в массиве
        .libs           rd 1 ; указатель на массив указателей на ассоциативные массивы библиотек
        .work_dir       rb 1024 ; max size path to work directory
        .work_dir.size  rd 1 ; length string
        .unit_dir       rb 1024
        .unit_dir.size  rd 1

        .MIME_types_arr rd 1
;;        .flags          dd 0 ; 1 - all hosts(элемент hosts не указатель на массив, а на функцию)

PATH:
        rb 256
; stack memory
        rb 4096
STACKTOP:
MEM:
;KOS_APP_END
