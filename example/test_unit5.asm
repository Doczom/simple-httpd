; test api 0.1.0 - get cmd path and get context unit
format MS COFF ;<- this is lib format
public @EXPORT as 'EXPORTS'

NO_DEBUG_INPUT = 0
include "macros.inc"
purge mov,add,sub
include "proc32.inc"

include '../module_api.inc'

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

        mov     eax, 1 ;no zero return - module init successful
.exit:
        pop     edi esi
        ret     8


server_entry:
        push    esi edi
        mov     esi, [esp + 4*2 + 4] ; request context

        invoke  IMPORT.create_resp, esi, FLAG_TRANSFER_CHUNKED\
                                       + FLAG_NO_CONTENT_LENGTH\
                                       + FLAG_NO_SERVER_HEADER\
                                       + FLAG_NO_CONTENT_ENCODING\
                                       + FLAG_NO_CONNECTION\
                                       + FLAG_NO_CACHE_CONTROL
        test    eax, eax
        jz      .exit

        push    eax
        mov     edi, eax
        invoke  IMPORT.begin_send_resp, edi, 0, 0

        invoke  IMPORT.send_resp, edi, text_no_cmd, text_no_cmd.size
        invoke  IMPORT.send_resp, edi, text_no_cmd2, text_no_cmd2.size
        invoke  IMPORT.send_resp, edi, text_no_cmd3, text_no_cmd3.size

        invoke  IMPORT.finish_send_resp, edi
        invoke  IMPORT.destruct_resp
.exit:
        pop     edi esi
        ret     8

server_close:

        ret     4


section '.data' data readable writable align 16

text_no_cmd:
        db 'chunk 1<br>'
.size = $ - text_no_cmd
text_no_cmd2:
        db 'chunk 2 - new size chunk<br>'
.size = $ - text_no_cmd2
text_no_cmd3:
        db 'chunk 3 - end chunk'
.size = $ - text_no_cmd3

@EXPORT:
export  \
        unit_init,       'httpd_init', \
        server_entry,    'httpd_serv',\
        server_close,    'httpd_close'


IMPORT IMPORT_DATA