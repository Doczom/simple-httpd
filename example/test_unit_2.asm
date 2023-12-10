format MS COFF
public @EXPORT as 'EXPORTS'

API_VERSION     = 0x05
FLAG_KEEP_ALIVE = 0x01

NO_DEBUG_INPUT = 0
include 'D:\kos\programs\macros.inc'
purge mov,add,sub
include 'D:\kos\programs\proc32.inc'

struct IMPORT_DATA
        version         rd 1 ; dword for check api
        sizeof          rd 1 ; size struct
        netfunc_socket  rd 1
        netfunc_close   rd 1
        netfunc_bind    rd 1
        netfunc_accept  rd 1
        netfunc_listen  rd 1
        netfunc_recv    rd 1
        netfunc_send    rd 1
        FileInfo        rd 1
        FileRead        rd 1
        Alloc           rd 1
        Free            rd 1
        parse_http_query rd 1
        ;send_resp(RESPD* ptr, char* content, uint32_t length)
        ;create_resp(CONNECT_DATA* session, uint32_t flags)
        ;       FLAG_KEEP_ALIVE = 0x01
        ;       FLAG_ADD_DATE   = 0x02 ;(not supported)
        ;       FLAG_NO_SET_CACHE  = 0x04 ;(not supported)
        ;       FLAG_NO_CONTENT_ENCODING = 0x08 ;(not supported)
        ;       
        ;destruct_resp(RESPD* ptr)
        ;set_http_status(RESPD* ptr, uint32_t status) ; status in '200' format, 
        ;add_http_header(RESPD* ptr, char* ptr_header)
        ;del_http_header(RESPD* ptr, char* ptr_header) ; no del std header
        ;set_http_ver(RESPD* ptr, char* version) ; example: RTSP/1.1
        send_resp       rd 1
        create_resp     rd 1
        destruct_resp   rd 1
        set_http_status rd 1
        add_http_header rd 1
        del_http_header rd 1
        set_http_ver    rd 1
        
        base_response   rd 1
        GLOBAL_DATA     rd 1
ends

struct CONNECT_DATA ; 16*4 = 64 bytes
        socket          dd 0 ; номер сокета подключения
        sockaddr        dd 16/4 ; socaddr connection
        buffer_request  dd 0 ; pointer to buffer for geting message socket
        request_size    dd 0 ; size geted data from client
        end_buffer_request dd 0 ; для парсера 
        buffer_response dd 0 ; pointer to buffwr for resp message 
        http_method     dd 0 ; указатель на строку
        http_verion     dd 0 ; указатель на строку
        num_headers     dd 0 ; number items in REQUEST_DATA
        http_headers    dd 0 ; указатель на массив REQUEST_DATA
        uri_scheme      dd 0 ; указатель на схему
        uri_authority   dd 0 ; pointer to struct ?
        uri_path        dd 0 ; указатель на декодированный путь к ресурсу(без параметров)
        num_uri_args    dd 0 ;
        uri_arg         dd 0 ; pointer to array REQUEST_DATA аргументов uri строк
        uri_fragment    dd 0 ; указатель на строку
        message_body    dd 0 ; указатель на тело http запроса
ends

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
        mov     eax, -1
        push    esi edi
        mov     esi, [esp + 4*2 + 4]

        cmp     dword[esi + IMPORT_DATA.version], API_VERSION
        jne     .exit

        mov     edi, IMPORT
        mov     ecx, [esi + IMPORT_DATA.sizeof]
        shr     ecx, 2 ; div 4
        rep movsd

        xor     eax, eax
.exit:
        pop     edi esi
        ret     4


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
        ret     4

.send_data: ; eax - ptr to buffer
        mov     edi, eax
        board_input 'create_resp'
        invoke  IMPORT.create_resp, esi, 0
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
        invoke  IMPORT.create_resp, esi, 0
        test    eax, eax
        jz      .exit
        
        mov     edi, eax
        invoke  IMPORT.set_http_status, edi, dword '404'
        invoke  IMPORT.send_resp, edi, 0, 0
        invoke  IMPORT.destruct_resp, edi
        jmp     .exit



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
        server_entry,    'httpd_serv'


IMPORT IMPORT_DATA ; 