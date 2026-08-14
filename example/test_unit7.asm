; test api 0.1.0 - get cmd path and get context unit
format MS COFF ;<- this is lib format
public @EXPORT as 'EXPORTS'

NO_DEBUG_INPUT = 0
include "macros.inc"
include "proc32.inc"

include '../module_api.inc'

section '.flat' code readable align 16

proc unit_init stdcall, .import:dword, .cmdline:dword,\
                        .uri_path:dword, .host:dword,\
                        .module_path:dword
        xor     eax, eax
        push    esi edi
        mov     esi, [.import]

        cmp     dword[esi + IMPORT_DATA.version], API_VERSION
        jne     .exit

        mov     edi, IMPORT
        mov     ecx, [esi + IMPORT_DATA.sizeof]
        shr     ecx, 2 ; div 4
        rep movsd

        ; create unit context
        mov     eax, [.uri_path]
        ;unit init successful
.exit:
        pop     edi esi
        ret
endp


proc server_entry stdcall, srv_context:dword, pdata:dword
        push    esi edi ebp
        mov     esi, [srv_context] ; request context

        sub     esp, 8*32
        mov     edx, esp

        ; get array substr
        mov     ecx, [pdata]
        invoke  IMPORT.get_matched_substr, [esi + CONNECT_DATA.uri_path], [pdata], edx, 8*32

        cmp     eax, -1
        je      .err_parse

        mov     esi, esp ; base array
        ; calc buffer size
        ; <p>count substr: 00</p><ol> <li></li> </ol>
        ; 27 + sum(4 + i.length + 5) + 5
        mov     edx, 27 + 5
        mov     ecx, 0
.loop_sum:
        cmp     ecx, eax
        jae     .end_sum_loop
        cmp     ecx, 32
        jae     .end_sum_loop

        add     edx, [esp + ecx*8 + 4]
        add     edx, 4+5
        inc     ecx
        jmp     .loop_sum
.end_sum_loop:
        add     edx, not -4
        and     edx, -4
        sub     esp, edx

        mov     dword[esp + edx - 4], '    '

        xor     ecx, ecx
        mov     dword[esp], '<p>c'
        mov     dword[esp + 4], 'ount'
        mov     dword[esp + 8], ' sub'
        mov     dword[esp + 12], 'str:'
        mov     dword[esp + 16], ' 00<'
        mov     dword[esp + 20], '/p><'
        mov     dword[esp + 24], 'ol> '
        lea     edi, [esp + 28]  ; str base

        cmp     eax, 0
        jz      .end_loop

        push    eax edx
        xor     edx, edx
        div     dword[__10__]
        shl     edx, 8
        add     edx, eax
        add     [esp + 4*2 + 16 + 1], dx
        pop     edx eax

        xor     ecx, ecx ; eax - count item
        cmp     eax, 32
        jbe     .loop_add

        mov     eax, 32
.loop_add:
        mov     dword[edi], '<li>'
        add     edi, 4
        push    esi
        mov     ecx, [esi + 4]
        mov     esi, [esi]
        test    ecx, ecx
        jz      @f
        rep movsb
@@:
        pop     esi

        mov     dword[edi], '</li'
        mov     byte[edi + 4], '>'
        add     edi, 5
        add     esi, 4*2
        dec     eax
        jnz     .loop_add

.end_loop:
        mov     dword[edi], '</ol'
        mov     byte[edi + 4], '>'

        mov     edi, esp
        mov     esi, edx

        invoke  IMPORT.create_resp, [srv_context], 0
        test    eax, eax
        jz      .exit2

        push    eax
        invoke  IMPORT.send_resp, eax, edi, esi
        invoke  IMPORT.destruct_resp
.exit2:
        add     esp, esi
.exit:
        add     esp, 8*32
        pop     ebp edi esi
        ret
.err_parse:
        invoke  IMPORT.create_resp, esi, 0
        push    eax
        invoke  IMPORT.send_resp, eax, err_text , err_text.len
        invoke  IMPORT.destruct_resp

        jmp     .exit
endp

proc server_close stdcall, pdata:dword
        ret
endp


section '.data' data readable writable align 16

@EXPORT:
export  \
        unit_init,       'httpd_init', \
        server_entry,    'httpd_serv',\
        server_close,    'httpd_close'

err_text: db '<html><body><b>ERROR!!!</b><p>get_replace_substr returned -1</p></body></html>'
.len = $ - err_text

__10__: dd 10

IMPORT IMPORT_DATA