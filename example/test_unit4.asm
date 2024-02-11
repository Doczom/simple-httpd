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
        ret     8


server_entry:
        push    esi edi
        mov     esi, [esp + 4*2 + 4] ; request context

        mov     edi, [esp + 4*2 + 8] ; unit context
        cmp     dword[edi], 0
        je      .no_cmd

        add     edi, 4
        invoke  IMPORT.create_resp, esi, 0
        test    eax, eax
        jz      .exit

        push    eax
        invoke  IMPORT.send_resp, eax, edi, [edi - 4]
        invoke  IMPORT.destruct_resp ; arg in stack
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
.size = $ - $$

@EXPORT:
export  \
        unit_init,       'httpd_init', \
        server_entry,    'httpd_serv',\
        server_close,    'httpd_close'


IMPORT IMPORT_DATA