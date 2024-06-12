; test api 0.1.0 - get cmd path and get context unit
format MS COFF ;<- this is lib format
public @EXPORT as 'EXPORTS'

NO_DEBUG_INPUT = 0
include "macros.inc"
purge mov,add,sub
include "proc32.inc"

include '../module_api.inc'

section '.flat' code readable align 16

; cmdline: <redirect uri path>

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
        mov     dword[eax + 4], 'Loca'
        mov     dword[eax + 8], 'tion'
        mov     word[eax + 12], ': '
        mov     edi, eax
        add     edi, 14

        mov     esi, [esp + 4*2 + 8]
@@:
        inc     esi
        cmp     byte[esi - 1], ' '
        je      @b
        dec     esi
@@:
        cmp     byte[esi], 0
        jz      @f
        movsb
        jmp     @b
@@:
        sub     edi, eax
        sub     edi, 4
        mov     [eax], edi
        ;unit init successful
.exit:
        pop     edi esi
        ret     12


server_entry:
        push    esi edi
        mov     esi, [esp + 4*2 + 4] ; request context

        mov     edi, [esp + 4*2 + 8] ; unit context
        add     edi, 4

        invoke  IMPORT.create_resp, esi, FLAG_NO_CONTENT_ENCODING\
                                       + FLAG_NO_CONTENT_TYPE\
                                       + FLAG_NO_CACHE_CONTROL
        test    eax, eax
        jz      .exit

        mov     esi, eax

        invoke  IMPORT.set_http_status, esi, dword '301'
        invoke  IMPORT.add_http_header, esi, edi, [edi - 4]

        invoke  IMPORT.begin_send_resp, esi, 0, 0
        invoke  IMPORT.destruct_resp, esi
.exit:
        pop     edi esi
        ret     8

server_close:
        push    dword[esp + 4]
        invoke  IMPORT.Free
        ret     4


section '.data' data readable writable align 16

@EXPORT:
export  \
        unit_init,       'httpd_init', \
        server_entry,    'httpd_serv',\
        server_close,    'httpd_close'


IMPORT IMPORT_DATA