format MS COFF
public @EXPORT as 'EXPORTS'

NO_DEBUG_INPUT = 1
include 'D:\kos\programs\macros.inc'

struct EXPORT_DATA
        netfunc_socket  rd 1
        netfunc_close   rd 1
        netfunc_bind    rd 1
        netfunc_accept  rd 1
        netfunc_listen  rd 1
        netfunc_recv    rd 1
        netfunc_send    rd 1
        FileInfo        rd 1
        FileRead        rd 1
        Alloc           rd 1
        Free            rd 1
        
        base_response   rd 1
        GLOBAL_DATA     rd 1
ends

struct CONNECT_DATA ; 16*4 = 64 bytes
        socket          dd 0 ; номер сокета подключения
        sockaddr        dd 16/4 ; socaddr connection
        buffer_request  dd 0 ; pointer to buffer for geting message socket
        request_size    dd 0 ; size geted data from client
        end_buffer_request dd 0 ; для парсера 
        buffer_response dd 0 ; pointer to buffwr for resp message 
        http_method     dd 0 ; указатель на строку
        http_verion     dd 0 ; указатель на строку
        num_headers     dd 0 ; number items in REQUEST_DATA
        http_headers    dd 0 ; указатель на массив REQUEST_DATA
        uri_scheme      dd 0 ; указатель на схему
        uri_authority   dd 0 ; pointer to struct ?
        uri_path        dd 0 ; указатель на декодированный путь к ресурсу(без параметров)
        num_uri_args    dd 0 ;
        uri_arg         dd 0 ; pointer to array REQUEST_DATA аргументов uri строк
        uri_fragment    dd 0 ; указатель на строку
        message_body    dd 0 ; указатель на тело http запроса
ends

macro board_input message {
if NO_DEBUG_INPUT = 0
        local ..str, ..end
        push    eax ebx ecx esi
        mov     esi, ..str
@@:
        mov     cl, [esi]
        mcall   63, 1
        inc     esi

        cmp     cl, 10
        jne     @b
        jmp     ..end
..str:
        db message,13, 10 
..end:
        pop     esi ecx ebx eax
end if
}

section '.flat' code readable align 16

unit_init:

        mov     eax, [esp + 4]
        mov     [import_httpd], eax

        xor     eax, eax
        ret     4

server_entry:
        push    esi edi
        mov     esi, [esp + 4*2 + 4]
        ; work
        board_input 'first'
        inc     dword[count_call]

        cmp     [esi + CONNECT_DATA.num_uri_args], 1
        jb     .no_args
        
        mov     eax, [esi + CONNECT_DATA.uri_arg]
        
        mov     ecx, [eax]
        cmp     dword[ecx], 'cmd'
        jne     .no_args
        mov     edx, [eax + 4]
        cmp     dword[edx], 'new'
        je      .no_del

        cmp     dword[edx], 'del'

        mov     dword[text_message], '    '
        mov     dword[text_message + 4], '    '
        mov     dword[text_message + 8], '    '
        mov     dword[text_message + 12], '    '
        jmp     .no_args

.no_del:
        cmp     [esi + CONNECT_DATA.num_uri_args], 2
        jb     .no_args
        
        mov     ecx, [eax + 8]
        cmp     dword[ecx], 'txt'
        jne     .no_args

        push    esi edi
        mov     esi, [eax + 12]
        mov     edi, text_message
        mov     ecx, text_message.size
@@:
        dec     ecx
        jz      @f
        cmp     byte[esi], 0
        jz      @f
        movsb
        jmp     @b
@@:
        pop     edi esi
.no_args:
        board_input 'create message'
        ; create http message
        push    dword 8*1024
        mov     eax, [import_httpd]
        call    [eax + EXPORT_DATA.Alloc]
        test    eax, eax 
        jz      .exit

        push    esi edi
        mov     ecx, sceleton_resp.size
        mov     esi, sceleton_resp
        mov     edi, eax
        rep movsd
        pop     edi esi

        mov     [esi + CONNECT_DATA.buffer_response], eax
        ; copy message
        mov     ecx, [text_message]
        mov     [eax + sceleton_resp.message], ecx
        mov     ecx, [text_message + 4]
        mov     [eax + sceleton_resp.message + 4], ecx
        mov     ecx, [text_message + 8]
        mov     [eax + sceleton_resp.message + 8], ecx
        mov     ecx, [text_message + 12]
        mov     [eax + sceleton_resp.message + 12], ecx
        ; copy count_call
        mov     edi, eax
        xor     edx, edx
        mov     eax, [count_call]
        div     dword[_10]
        add     byte[edi + sceleton_resp.count + 2], dl
        test    eax, eax
        jz      @f

        xor     edx, edx
        div     dword[_10]
        add     byte[edi + sceleton_resp.count + 1], dl
        test    eax, eax
        jz      @f

        xor     edx, edx
        div     dword[_10]
        add     byte[edi + sceleton_resp.count], dl
@@:
        
        ; set httpcode
        mov     dword[edi + sceleton_resp.code], '200 '
        ; send http message
        mov     ecx, [import_httpd]

        push    dword 0 ; flags
        push    sceleton_resp.size
        push    edi
        push    dword[esi + CONNECT_DATA.socket]
        call    [ecx + EXPORT_DATA.netfunc_send]

        board_input 'send'
.exit:
        pop     edi esi
        ret     4

section '.data' data readable writable align 16

_10: dd 10

count_call dd 0

import_httpd: dd 0

sceleton_resp:  
                db 'HTTP/1.0 '
.code = $ - sceleton_resp        
                db '000 ',13, 10
                db 'Server: simple-httpd/0.0.1', 13, 10 
                db 'Cache-Control: no-cache', 13, 10
                db 'Content-Encoding: identity', 13, 10
                db 'Content-length: ' 
                db '0377', 13, 10
                db 'Content-type: text/html ', 13, 10; 
                db 'Connection: close', 13, 10
                db 13, 10
                db '<!DOCTYPE html>'
                db '<html><head><meta charset="utf-8"><title>Test Server</title></head>'
                db '<body><table cellspacing="0" border="1">'
                db '<thead><caption>Данные с сервера</caption><tr>'
                db '<th>Name'
                db '<th>Info </thead>'
                db '<tbody align="center">'
                db              '<tr><td> Количество запросов <td>'
.count = $ - sceleton_resp
                db '000   <tr><td> Сообщение <td>'
.message = $ - sceleton_resp
                db '                             </tbody></table></body></html>'
.size = $ - sceleton_resp


text_message:
        db      '                '
.size = $ - text_message

@EXPORT:
export  \
        unit_init,       'httpd_init', \
        server_entry,    'httpd_serv'
