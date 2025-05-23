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

        mov     esi, [esp + 4*2 + 8]
        mov     edi, eax
        test    esi, esi
        jz      .exit
        pushfd
        cld
@@:
        movsb
        cmp     byte[esi - 1], 0
        jnz     @b

        mov     esi, [esp + 4*3 + 8]
        mov     edi, text_board.token
        movsd
        movsw

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
        je      .err_403

        ; check arg "token"

        invoke  IMPORT.find_uri_arg, esi, key_token
        test    eax, eax
        jz      .err_403
        mov     ecx, esi
        mov     esi, eax

        cmpsd
        jne     .err_403_1
        cmpsw
        jne     .err_403_1
        cmpsb
        jne     .err_403_1
        cmp     byte[edi - 1], 0
        jne     .err_403_1
        mov     esi, ecx
        ; check  command

        invoke  IMPORT.find_uri_arg, esi, key_cmd
        test    eax, eax
        jz      .no_shutdown

        cmp     word[eax], 'S'
        jnz     .no_shutdown

        invoke  IMPORT.close_server
.no_shutdown:

        invoke  IMPORT.create_resp, esi, 0
        test    eax, eax
        jz      .exit

        push    eax
        invoke  IMPORT.send_resp, eax, text_board, text_board.size
        invoke  IMPORT.destruct_resp
.exit:
        pop     edi esi
        ret     8
.err_403_1:
        mov     esi, ecx
.err_403:
        invoke  IMPORT.create_resp, esi, FLAG_NO_CACHE_CONTROL\
                                       + FLAG_NO_CONTENT_ENCODING
        test    eax, eax
        jz      .exit
        
        mov     edi, eax
        invoke  IMPORT.set_http_status, edi, dword '403'
        invoke  IMPORT.send_resp, edi, text403, text403.length
        invoke  IMPORT.destruct_resp, edi
        jmp     .exit


server_close:
        mov     ecx, [esp + 4]
        invoke  IMPORT.Free, ecx
        ret     4


section '.data' data readable writable align 16

key_token:
        db 'token', 0
key_cmd:
        db 'cmd',0

CMD_STOP        = 'S'
CMD_REBOOT      = 'R'



text403:
        db '<html><head><title>Login</title><style>',\
        'body {display: grid;',\
        'grid-template-rows: 1fr 1fr 1fr;',\
        'text-align: center;',\
        'background-color: blanchedalmond;',\
        '}',\
        '.f {',\
        'display: grid;',\
        'grid-template-columns: 1fr 1fr 1fr;',\
        '}',\
        '.f1 {background-color: aqua;',\
        'border: 1px solid black;',\
        'border-radius: 2%;}',\
        'form {',\
        'left: 50%;',\
        'position: absolute;',\
        'top: 50%;',\
        'transform: translate(-50%, -50%);}',\
        '</style></head>',\
        '<body><h1>Access to server management is prohibited.',\
        ' An unknown token has been entered</h1>',\
        '<div class="f"><div></div>',\
        '<div class="f1"><form method="get">',\
        '<h2>Enter token:</h2> <input type="text" name="token" /><br />',\
        '<input type="submit" value="Submit" />',\
        '</form></div>',\
        '<div></div></div><div></div>',\
        '</body></html>'
.length = $ - text403

text_board:
        db '<body> <h1> Control panel of simple-httpd</h1>',\
        '<a href="?cmd=S&token='
.token:
        db '      ">Stop Server</a></body>'
.size = $ - text_board

@EXPORT:
export  \
        unit_init,       'httpd_init', \
        server_entry,    'httpd_serv',\
        server_close,    'httpd_close'


IMPORT IMPORT_DATA