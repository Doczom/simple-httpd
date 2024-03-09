format MS COFF
public @EXPORT as 'EXPORTS'

NO_DEBUG_INPUT = 0
include "macros.inc"
purge mov,add,sub
include "proc32.inc"

include '../module_api.inc'

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

        cmp     [esi + CONNECT_DATA.num_uri_args], 1
        jb     .no_args
        
        mov     eax, [esi + CONNECT_DATA.uri_arg]
        
        mov     ecx, [eax]
        cmp     word[ecx], 'gr'
        jne     .no_args

        cmp     byte[ecx + 2], 0
        jne     .no_args

        
        mov     ecx, [eax + 4]
        cmp     dword[ecx], 'bpo'
        jne     .no_bpo

        board_input 'bpo'
        invoke  IMPORT.Alloc, sceleton_resp.size
        test    eax, eax
        jz      .exit

        push    esi
        mov     edi, eax
        mov     esi, sceleton_resp
        mov     ecx, sceleton_resp.size
        rep movsb

        lea     edi, [eax + sceleton_resp.name]
        mov     esi, bpo_name
        mov     ecx, bpo_name.size
        rep movsb

        lea     edi, [eax + sceleton_resp.data]
        mov     esi, bpo_data
        mov     ecx, bpo_data.size
        rep movsb
        pop     esi

        jmp     .send_data
.no_bpo:
        cmp     dword[ecx], 'btp'
        jne     .err_404

        board_input 'btp'
        invoke  IMPORT.Alloc, sceleton_resp.size
        test    eax, eax
        jz      .exit

        push    esi
        mov     edi, eax
        mov     esi, sceleton_resp
        mov     ecx, sceleton_resp.size
        rep movsb

        lea     edi, [eax + sceleton_resp.name]
        mov     esi, btp_name
        mov     ecx, btp_name.size
        rep movsb

        lea     edi, [eax + sceleton_resp.data]
        mov     esi, btp_data
        mov     ecx, btp_data.size
        rep movsb
        pop     esi

        jmp     .send_data
.no_args:
        board_input 'no_arg'
        invoke  IMPORT.create_resp, esi, 0
        test    eax, eax
        jz      .exit

        push    eax
        invoke  IMPORT.send_resp, eax, sceleton_resp, sceleton_resp.size
        invoke  IMPORT.destruct_resp ; arg in stack
.exit:
        pop     edi esi
        ret     8

.send_data: ; eax - ptr to buffer
        mov     edi, eax
        board_input 'create_resp'
        invoke  IMPORT.create_resp, esi, FLAG_KEEP_ALIVE
        test    eax, eax
        jz      .exit

        board_input 'send_data'
        push    eax
        invoke  IMPORT.send_resp, eax, edi, sceleton_resp.size
        invoke  IMPORT.destruct_resp ; arg in stack

        invoke  IMPORT.Free, edi
        jmp     .exit

.err_404:
        ; send resp 404
        board_input 'err404'
        invoke  IMPORT.create_resp, esi, FLAG_NO_CACHE_CONTROL\
                                       + FLAG_NO_CONTENT_ENCODING
        test    eax, eax
        jz      .exit
        
        mov     edi, eax
        invoke  IMPORT.set_http_status, edi, dword '404'
        invoke  IMPORT.send_resp, edi, 0, 0
        invoke  IMPORT.destruct_resp, edi
        jmp     .exit

server_close:

        ret     4

section '.data' data readable writable align 16

_10: dd 10

sceleton_resp:  
                db '<!DOCTYPE html>'
                db '<html><head><meta charset="utf-8"><title>Test Server 2</title></head>'
                db '<body><ul><li><a href="?gr=bpo">bpo</a></li><li><a href="?gr=btp">btp</a></li></ul>' 
                db '<b>Название группы: </b>'
.name = $ - sceleton_resp
                db '                                  <br><b>Экзамены:</b>'
.data = $ - sceleton_resp
                db '                                                                  <br></body></html>'
.size = $ - sceleton_resp


bpo_data:
        db      'Дискретка :('
.size = $ - bpo_data
bpo_name:
        db      'Разарботчики'
.size = $ - bpo_name

btp_data:
        db      'Эти химию сдают :)'
.size = $ - btp_data
btp_name:
        db      'Технологи'
.size = $ - btp_name

@EXPORT:
export  \
        unit_init,       'httpd_init', \
        server_entry,    'httpd_serv',\
        server_close,    'httpd_close'


IMPORT IMPORT_DATA ; 