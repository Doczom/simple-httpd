

struct CONNECT_DATA ; 16*4 = 64 bytes
        socket          dd 0 ; номер сокета подключения
        sockaddr        dd 16/4 ; socaddr connection
        buffer_request  dd 0 ; pointer to buffer for geting message socket
        request_size    dd 0 ; size geted data from client
        tmp_req_size    dd 0 ; для парсера 
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

struct REQUEST_DATA
        ptr_name        dd 0 ;
        ptr_data        dd 0 ;
ends

; Load server config 
; ecx - path to file
; OUT: eax - 0 or err_code
load_settings:
        ; check file path
        ;sub     esp, 40 ; size file info struct
        ;push    esp
        ;push    ecx
        ;call    FileInfo
        ;lea     esp, [esp + 40]
        ;test    eax, eax 
        ;jnz     .err

        ; TEST SERVER
        mov     word[srv_sockaddr], AF_INET4
        mov     word[srv_sockaddr.port], 0x5000 ; 80 port
        mov     dword[srv_sockaddr.ip],  0x1589A8C0 ; 192.168.137.21 or 0xc0a88915 ?
        mov     dword[srv_backlog], 10
        
        push    esi edi
        mov     edi, GLOBAL_DATA.work_dir
        mov     esi, test_workdir
        mov     ecx, test_workdir.size
        rep movsb
        pop     edi esi  
        mov     dword[GLOBAL_DATA.work_dir.size], test_workdir.size


        mov     dword[GLOBAL_DATA.MIME_types_arr], STD_MIME_TYPE_ARR

        xor     eax, eax
        ret
.err:
        ret

test_workdir: db '/sys'
.size = $ - test_workdir


; Config format:
; Standart INI file:
;       - ";" or "#" comments
;       - [name] name of group 
;       - arg=val params in group  


