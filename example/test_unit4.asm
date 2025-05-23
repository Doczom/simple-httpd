; test api 0.1.0 - get cmd path and get context unit
format MS COFF ;<- this is lib format
public @EXPORT as 'EXPORTS'

NO_DEBUG_INPUT = 0
include "macros.inc"
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

        ; create unit context
        invoke  IMPORT.Alloc, 4096 ; for cmd path
        test    eax, eax
        jz      .exit

        mov     dword[eax], 0
        mov     esi, [esp + 4*2 + 8]
        lea     edi, [eax + 4]
        test    esi, esi
        jz      .exit
        pushfd
        cld
@@:
        cmp     byte[esi], 0
        jz      @f
        movsb
        inc     dword[eax]
        jmp     @b
@@:
        popfd
        ;unit init successful
.exit:
        pop     edi esi
        ret     12


server_entry:
        push    esi edi
        mov     esi, [esp + 4*2 + 4] ; request context

        mov     edi, [esp + 4*2 + 8] ; unit context
        cmp     dword[edi], 0
        je      .no_cmd

        mov     edx, [esi + CONNECT_DATA.uri_path]
        xor     ecx, ecx
        dec     ecx
@@:
        inc     ecx
        cmp     byte[edx + ecx], 0
        jne     @b
        push    ecx

        add     edi, 4
        invoke  IMPORT.create_resp, esi, FLAG_TRANSFER_CHUNKED\
                                       + FLAG_NO_CONTENT_LENGTH
        test    eax, eax
        jz      .exit

        push    eax
        invoke  IMPORT.begin_send_resp, eax, 0, 0
        mov     eax, [esp]
        invoke  IMPORT.send_resp, eax, edi, [edi - 4]
        mov     eax, [esp]
        invoke  IMPORT.send_resp, eax, text_br, text_br.size
        mov     eax, [esp]
        invoke  IMPORT.send_resp, eax,\
                                  [esi + CONNECT_DATA.uri_path],\
                                  [esp + 4]
        mov     eax, [esp]
        invoke  IMPORT.finish_send_resp, eax
        invoke  IMPORT.destruct_resp ; arg in stack
        add     esp, 4
.exit:
        pop     edi esi
        ret     8
.no_cmd:
        invoke  IMPORT.create_resp, esi, 0
        test    eax, eax
        jz      .exit

        push    eax
        invoke  IMPORT.send_resp, eax, text_no_cmd, text_no_cmd.size
        invoke  IMPORT.destruct_resp
        jmp     .exit

server_close:

        ret     4


section '.data' data readable writable align 16

text_no_cmd:
        db 'For this unit in config not set arguments'
.size = $ - text_no_cmd
text_br:
        db '<br>'
.size = $ - text_br

@EXPORT:
export  \
        unit_init,       'httpd_init', \
        server_entry,    'httpd_serv',\
        server_close,    'httpd_close'


IMPORT IMPORT_DATA