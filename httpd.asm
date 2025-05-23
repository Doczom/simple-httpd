;*****************************************************************************;
;  Copyright (C) 2023-2024, Mikhail Frolov aka Doczom . All rights reserved.  ;
;            Distributed under terms of the 3-Clause BSD License.             ;
;                                                                             ;
;                 httpd - Simple http server for Kolibri OS.                  ;
;                                                                             ;
;                         Version 0.2.5, 13 June 2024                         ;
;                                                                             ;
;*****************************************************************************;

HTTPD_FIRST_REQUEST_BUFFER      = 0x8000


;include "macros.inc"
  use32
  org    0
  db     'MENUET01'
  dd     1, START, I_END, MEM, STACKTOP
M01header.params:
  dd     PATH, 0
include "macros.inc"

include 'module_api.inc'

include "proc32.inc"
include "dll.inc"
include "KOSfuncs.inc"
;include 'D:\kos\programs\network.inc'
;KOS_APP_START

include 'sys_func.inc'
include 'settings.inc'

;CODE
START:
        call    InitHeap
        mcall   SF_SET_EVENTS_MASK, EVM_STACK ;set event bitmap

        ; init library
        stdcall dll.Load, @IMPORT
        test    eax, eax
        jnz     .err_settings

        mov     ecx, default_ini_path
        mov     eax, [M01header.params]
        cmp     byte[eax],0
        jz      .load_settings

        mov     edx, ' '
        mov     ecx, eax
        cmp     byte[eax], '"'
        jne     @f
        mov     dl, '"'
        inc     ecx
@@:
        mov     esi, ecx
@@:
        lodsb
        test    al, al
        jz      @f
        cmp     al, dl
        jne     @b
@@:
        mov     byte[esi - 1], 0
.load_settings:
        ; get settings
        call    load_settings ; ecx -> string to config file
        test    eax, eax
        jnz     .err_settings

        ;init server socket
        push    dword SO_NONBLOCK ;IPPROTO_TCP
        push    dword SOCK_STREAM
        push    dword AF_INET4
        call    netfunc_socket; AF_INET4, SOCK_STREAM, SO_NONBLOCK ; we dont want to block on accept
        cmp     eax, -1
        je      .sock_err
        mov     [srv_socket], eax

        stdcall netfunc_bind, [srv_socket], srv_sockaddr, srv_sockaddr.length
        cmp     eax, -1
        je      .bind_err

        ; listen()
        stdcall netfunc_listen, [srv_socket], [srv_backlog]
        cmp     eax, -1
        jz      .listen_err

.mainloop:
        cmp     dword[srv_shutdown], 1
        jz      .shutdown

        mcall   23, 100 ; get event to network stack
        test    eax, eax
        jz      .mainloop

        cmp     dword[srv_stop], 1
        jz      .mainloop

        push    dword thread_connect
        call    CreateThread ; not save PID
        jmp     .mainloop

.shutdown:
.listen_err:
.bind_err:
        stdcall netfunc_close, [srv_socket]

.err_settings:
.sock_err:
        ThreadExit

;-----------------------------------------------------------------------------

thread_connect:
        sub     esp, sizeof.CONNECT_DATA
        mcall   SF_SET_EVENTS_MASK, EVM_STACK ; set event - network event

        ; Accept - wait connection, get socket and sockaddr
        lea     edx, [esp + CONNECT_DATA.sockaddr] ; new sockaddr
        push    sizeof.sockaddr_in
        push    edx
        push    dword[srv_socket]
        call    netfunc_accept

        cmp     eax, -1
        jz      .err_accept

        ; connection is enable
        mov     [esp + CONNECT_DATA.socket], eax

        ; теперь нужно выделить буфер(тоже на 16 кб наверное), и
        ; прочитать в этот буфер из сокета, когда прочтём ноль(или больше 4 кб), тогда
        ; выходим из цикла и анализируем заголовки и стартовую стоку.
        stdcall Alloc, HTTPD_FIRST_REQUEST_BUFFER ;alloc memory 32 kib
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
        push    dword HTTPD_FIRST_REQUEST_BUFFER
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
 ;       cmp     [esi + CONNECT_DATA.request_size], HTTPD_FIRST_REQUEST_BUFFER ; check end buffer
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
        ; find unit  for uri path
        cmp     dword[GLOBAL_DATA.modules], 0
        jz      .no_modules

        mov     eax, [GLOBAL_DATA.modules]
.next_unit:
        push    esi edi
        mov     esi, [esi + CONNECT_DATA.uri_path]
        lea     edi, [eax + HTTPD_MODULE.uri_path]
        ; check module for all uri path
@@:
        cmpsb
        jne     @f

        cmp     byte[edi - 1], 0
        jne     @b
.found_module:
        ; found module
        pop     edi esi

        push    dword[eax + HTTPD_MODULE.pdata] ; context of module
        push    esi                           ; coutext of request
        call    dword[eax + HTTPD_MODULE.httpd_serv] ; call unit function

        jmp     .end_work
@@:
.found_special_char:
        cmp     byte[edi - 1], '*'
        jne     .no_special_char

        ;je      .found_module
        ; check next char in uri of modules
        cmp     byte[edi], 0
        je      .found_module
        ; uri path "*name" or "*name*"
        dec     esi
.special_char_loop:
        cmp     byte[esi], 0
        je      .no_special_char

        ;inc     esi

        xor     ecx, ecx
@@:
        inc     ecx
        mov     dl, byte[edi + ecx - 1]

        cmp     dl, '*'
        je      @f

        cmp     byte[esi + ecx - 1], dl
        lea     esi, [esi + 1]
        jne     .special_char_loop
        dec     esi

        test    dl, dl
        jnz     @b

        jmp     .found_module
@@:
        ; found substr
        add     esi, ecx
        add     edi, ecx

        jmp     .found_special_char

.no_special_char:
        pop     edi esi

        mov     eax, [eax] ; HTTPD_MODULE.next
        test    eax, eax ; terminate list
        jne     .next_unit

.no_modules:
        ; if not found modules, call file_server
        call    file_server ; esi - struct
        ; end work thread
        jmp     .end_work


.err_parse:
        call    file_server.err_http_501
.end_work:
        add     esp, 2048
.err_recv_sock:
        ; free IN buffer
        cmp     dword[esp + CONNECT_DATA.buffer_request], 0
        jz      .err_alloc

        stdcall Free, [esp + CONNECT_DATA.buffer_request]
.err_alloc:
        stdcall netfunc_close, [esp + CONNECT_DATA.socket]
.err_accept:
        add     esp, sizeof.CONNECT_DATA
        ; close this thread
        ret

include 'parser.inc'

include 'file_server.inc'
; DATA AND FUNCTION
include 'httpd_lib.inc'


I_END:
;DATA

@IMPORT:
library libini,                 'libini.obj'

import  libini,\
        ini.get_str,            'ini_get_str',\
        ini.get_int,            'ini_get_int',\
        ini.enum_keys,          'ini_enum_keys'


default_ini_path: db 'httpd.ini',0

ini_section_units:      db 'MODULES',0
ini_section_main:       db 'MAIN', 0
ini_section_tls         db 'TLS',0

ini_key_ip              db 'ip',0
ini_key_port            db 'port',0
ini_key_conn            db 'conn',0
ini_key_flags           db 'flags',0
ini_key_work_dir        db 'work_dir',0
ini_key_modules_dir     db 'modules_dir',0
ini_key_mime_file       db 'mime_file',0

ini_key_mbedtls_obj     db 'mbedtls',0
ini_key_ca_cer          db 'ca_cer',0
ini_key_srv_crt         db 'srv_crt',0
ini_key_srv_key         db 'srv_key',0

httpd_module_init       db 'httpd_init',0
httpd_module_serv       db 'httpd_serv',0
httpd_module_close      db 'httpd_close',0

IMPORT_MODULE:
        dd      httpd_import, GLOBAL_DATA.modules_dir, 0

httpd_import:
.init   dd      httpd_module_init
.serv   dd      httpd_module_serv
.close  dd      httpd_module_close
        dd      0

EXPORT_DATA:    ; in modules for this table using struct IMPORT_DATA
        dd      API_VERSION
        dd      .size
        dd      netfunc_socket
        dd      netfunc_close
        dd      netfunc_bind
        dd      netfunc_accept
        dd      netfunc_listen
        dd      netfunc_recv
        dd      netfunc_send
        dd      Alloc
        dd      Free
        dd      parse_http_query ; no stdcall

        dd      FileInitFILED
        dd      FileInfo
        dd      FileRead
        dd      FileSetOffset
        dd      FileReadOfName

        dd      send_resp
        dd      create_resp
        dd      destruct_resp
        dd      set_http_status
        dd      add_http_header
        dd      del_http_header
        dd      set_http_ver
        dd      begin_send_resp
        dd      finish_send_resp

        dd      find_uri_arg
        dd      find_header
        dd      read_http_body
        dd      Get_MIME_Type
        dd      close_server

        dd      GLOBAL_DATA
.size = $ - EXPORT_DATA ; (count func)*4 + size(api ver) + 4
        dd      0
; DATA

;UDATA
srv_stop:       rd 1 ; set 1 for skip new connections
srv_shutdown:   rd 1 ; set 1 for ending working server
srv_tls_enable  rd 1 ; set 1 for enable TLS mode working

srv_backlog:    rd 1 ; maximum number of simultaneous open connections

srv_socket:     rd 1

srv_sockaddr:
                rw 1
  .port         rw 1
  .ip           rd 1
                rb 8
  .length       = $ - srv_sockaddr

GLOBAL_DATA:
        .modules        rd 1 ; pointer to a doubly connected non-cyclic list (null terminator)
                             ; next, prev, ptr of httpd_serv(), uri path
        .work_dir              rb 1024 ; max size path to work directory
        .work_dir.size         rd 1 ; length string
        .modules_dir           rb 1024
        .modules_dir.end       rd 1

        .MIME_types_arr        rd 1
        .flags                 rd 1
        ._module_cmd           rd 1

PATH:
        rb 256
; stack memory
        rb 4096
STACKTOP:
MEM:
;KOS_APP_END
