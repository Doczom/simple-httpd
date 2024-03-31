format MS COFF
public @EXPORT as 'EXPORTS'

include "macros.inc"

include '../module_api.inc'

NO_DEBUG_INPUT = 0

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
        xor     eax, eax
        push    esi edi
        mov     esi, [esp + 4*2 + 4]

        cmp     dword[esi + IMPORT_DATA.version], API_VERSION
        jne     .exit

        mov     edi, IMPORT
        mov     ecx, [esi + IMPORT_DATA.sizeof]
        shr     ecx, 2 ; div 4
        rep movsd

        mov     eax, 1 ;no zero return - unit init successful
.exit:
        pop     edi esi
        ret     8


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

        ;cmp     dword[edx], 'del'

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

        mov     dword[text_message], '    '
        mov     dword[text_message + 4], '    '
        mov     dword[text_message + 8], '    '
        mov     dword[text_message + 12], '    '

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
        call    [IMPORT.Alloc]
        test    eax, eax 
        jz      .exit

        push    esi edi
        mov     ecx, sceleton_resp.size
        mov     esi, sceleton_resp
        mov     edi, eax
        rep movsd
        pop     edi esi

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
        push    dword FLAG_RAW_STREAM
        push    esi
        call    [IMPORT.create_resp]

        test    eax, eax
        jz      .exit

        mov     esi, eax
        push    sceleton_resp.size
        push    edi
        push    eax
        call    [IMPORT.send_resp]

        push    esi
        call    [IMPORT.destruct_resp]

        push    edi
        call    [IMPORT.Free]

        board_input 'send'
.exit:
        pop     edi esi
        ret     8

server_close:

        ret     4


section '.data' data readable writable align 16

_10: dd 10

count_call dd 0

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
        server_entry,    'httpd_serv',\
        server_close,    'httpd_close'


IMPORT IMPORT_DATA ; 