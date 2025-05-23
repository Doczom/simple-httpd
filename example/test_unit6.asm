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
        mov     eax, 1
        ;unit init successful
.exit:
        pop     edi esi
        ret     12


server_entry:
        push    esi ebp
        mov     esi, [esp + 4*2 + 4] ; request context

        mov     ebp, esp
        push    dword '</p>'
        push    dword '    '

        mov     ecx, 10
        movzx   eax, word[esi + CONNECT_DATA.sockaddr.sin_port]
        xchg    al, ah
@@:
        xor     edx, edx
        div     ecx
        add     dl, '0'
        mov     byte[esp], dl
        add     esp, -1
        test    eax, eax
        jnz     @b

        inc     esp
        push    dword 'ort:'
        push    dword '   p'

        mov     ecx, 4
.loop:
        movzx   eax, byte[esi + CONNECT_DATA.sockaddr.sin_addr + ecx - 1]
@@:
        xor     edx, edx
        div     dword[__10]
        add     dl, '0'
        mov     byte[esp], dl
        add     esp, -1
        test    eax, eax
        jnz     @b

        mov     byte[esp], '.'
        add     esp, -1
        dec     ecx
        jnz     .loop

        add     esp, 2

        push    dword 'ip: '
        push    dword '<p> '

        invoke  IMPORT.create_resp, esi, 0
        test    eax, eax
        jz      .exit

        mov     edx, ebp
        mov     ecx, esp
        sub     edx, esp

        push    eax
        invoke  IMPORT.send_resp, eax, ecx, edx
        invoke  IMPORT.destruct_resp
        mov     esp, ebp
.exit:
        pop     ebp esi
        ret     8

server_close:

        ret     4


section '.data' data readable writable align 16

__10:   dd 10


@EXPORT:
export  \
        unit_init,       'httpd_init', \
        server_entry,    'httpd_serv',\
        server_close,    'httpd_close'


IMPORT IMPORT_DATA