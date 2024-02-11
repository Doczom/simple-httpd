;*****************************************************************************;
;                                                                             ;
;            Structures and constants for modules of simple-httpd             ;
;                                                                             ;
;*****************************************************************************;

API_VERSION     = 0x100 ; 0.1.0

FLAG_KEEP_ALIVE = 0x01
FLAG_ADD_DATE   = 0x02 ;(not supported)
FLAG_NO_SET_CACHE  = 0x04 ;(not supported)
FLAG_NO_CONTENT_ENCODING = 0x08 ;(not supported)
FLAG_TRASFER_CHUNKED = 0x10 ;(not supported)

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
        parse_http_query rd 1 ; not standart calling, not using this function
        send_resp       rd 1
        ; send_resp(RESPD* ptr, char* content, uint32_t length);
        create_resp     rd 1
        ; RESPD* create_resp(CONNECT_DATA* session, uint32_t flags);
        destruct_resp   rd 1
        ; void stdcall destruct_resp(RESPD* ptr);
        set_http_status rd 1
        ; void set_http_status(RESPD* ptr, uint32_t status);
        ;   status in '200' format
        add_http_header rd 1
        ; uint32_t add_http_header(RESPD* ptr, char* ptr_header, uint32_t length);
        del_http_header rd 1
        ; uint32_t del_http_header(RESPD* ptr, char* ptr_header);
        ;   no del std header
        set_http_ver    rd 1
        ; void set_http_ver(RESPD* ptr, char* version, uint32_t length);
        ;   example: 'RTSP/1.1 '
        find_uri_arg    rd 1
        ;char* find_uri_arg(CONNECT_DATA* session, char* key);
        find_header     rd 1
        ;char* find_header(CONNECT_DATA* session, char* key);
        close_server    rd 1
        ;void close_server();


        base_response   rd 1
        GLOBAL_DATA     rd 1
ends