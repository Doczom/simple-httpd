; test api 0.1.0 - get cmd path and get context unit
format MS COFF ;<- this is lib format
public @EXPORT as 'EXPORTS'

NO_DEBUG_INPUT = 0
include "macros.inc"
include "proc32.inc"

include '../module_api.inc'

section '.flat' code readable align 16

; cmdline: <http_code> <text for body http response>

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

        mov     esi, [esp + 4*2 + 8]
        test    esi, esi
        jnz     @f
        xor     ecx, ecx
        jmp     .alloc
@@:
        cmp     byte[esi], ' '
        jne     @f
        inc     esi
        jmp     @b
@@:
        xor     ecx, ecx
        dec     ecx
@@:
        inc     ecx
        cmp     byte[esi+ecx], 0
        jnz     @b
.alloc:
        add     ecx, 4
        push    ecx
        ; create unit context
        invoke  IMPORT.Alloc, ecx ; for cmd path
        pop     ecx
        test    eax, eax
        jz      .exit

        mov     edi, eax
        add     edi, 4
        sub     ecx, 4
        mov     [eax], ecx
        rep movsb

        ;unit init successful
.exit:
        pop     edi esi
        ret     12


server_entry:
        push    esi edi
        mov     esi, [esp + 4*2 + 4] ; request context

        mov     edi, [esp + 4*2 + 8] ; unit context
        add     edi, 4

        invoke  IMPORT.create_resp, esi, 0
        test    eax, eax
        jz      .exit

        mov     esi, eax

        mov     eax, '403'
        cmp     dword[edi - 4], 3
        jb      @f
        mov     eax, [edi]
@@:
        invoke  IMPORT.set_http_status, esi, eax

        lea     eax, [edi + 4]
        mov     ecx, [edi - 4]
        sub     ecx, 4
        cmp     dword[edi - 4], 5
        jae     @f

        xor     eax, eax
        xor     ecx, ecx
@@:
        invoke  IMPORT.send_resp, esi, eax, ecx
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