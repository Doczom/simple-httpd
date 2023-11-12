;*****************************************************************************;
;      Copyright (C) 2023, Mikhail Frolov aka Doczom . All rights reserved.   ;
;            Distributed under terms of the 3-Clause BSD License.             ;
;                                                                             ;
;                 httpd - Simple http server for Kolibri OS.                  ;
;                                                                             ;
;                      Version 0.0.1, 12 November 2023                        ;
;                                                                             ;
;*****************************************************************************;
use32
org 0


include 'sys_func.inc'
include 'settings.inc'


start:
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
        push    dword SO_NONBLOCK
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
        call    close; [srv_socket]

..err_settings:
.sock_err:
        mcall   -1



thread_connect:
        sub     esp, sizeof.CONNECT_DATA
        mcall   40, EVM_STACK ; set event bitmap - network event 

        ; ожидание подключения Accept, sockaddr находится на вершине стека нового потока
        lea     edx, [esp + CONNECT_DATA.sockaddr] ; new sockaddr
        push    dword 16 ; 16 byte - sockaddr length
        push    edx
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
        cmp     [esi + CONNECT_DATA.request_size], 0x8000 ; check end buffer
        jb      @b
@@:
        ; после получения всего запроса(более или менее всего) выделяем озу для 
        ; ассоциативного массива заголовков и аргументов запроса
        ; 8*50 + 8*100
        ;  esp        .. esp + 1024 -> for http headers
        ;  esp + 1024 .. esp + 2048 -> for URI args
        sub     esp, 2048 

        ; parse http message 
        call    parse_http_query ; ecx - buffer edx - length data in buffer
        test    eax, eax
        jz      .err_parse
        ; вызов нужной функции из списка моделей
        
        ; TODO
        
        ; end work thread
        jmp     .end_work
        

.err_parse:
        ; send error 501

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

; DATA AND FUNCTION
include 'httpd_lib.inc'

; DATA

srv_backlog:    dd 0 ; максимум одновременных подключений подключений

srv_socket:     dd 0

srv_sockaddr:
                dw AF_INET4
  .port         dw 0
  .ip           dd 0
                rb 8
  .length       = $ - srv_sockaddr

GLOBAL_DATA:
        .units          dd 0 ; указатель на ассоциативный массив пути и указателя на функцию либы(см ниж)
        .unit_count     dd 0 ; количество записей в массиве
        .libs           dd 0 ; указатель на массив указателей на ассоциативные массивы библиотек
;;        .flags          dd 0 ; 1 - all hosts(элемент hosts не указатель на массив, а на функцию)


