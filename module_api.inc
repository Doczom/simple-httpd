;*****************************************************************************;
;                                                                             ;
;            Structures and constants for modules of simple-httpd             ;
;                                                                             ;
;*****************************************************************************;

API_VERSION     = 0x100 ; 0.1.0

FLAG_KEEP_ALIVE          = 0x01
FLAG_NO_CONNECTION       = 0x02
FLAG_NO_SERVER_HEADER    = 0x04
FLAG_NO_CONTENT_ENCODING = 0x08
FLAG_NO_DATE             = 0x10 ;(not supported)

FLAG_NO_CONTENT_LENGTH   = 0x20
FLAG_NO_CONTENT_TYPE     = 0x40
FLAG_NO_CACHE_CONTROL    = 0x80

FLAG_TRANSFER_CHUNKED    = 0x100
FLAG_RAW_STREAM          = 0x200

struct sockaddr_in
        sin_family      dw      ?       ; sa_family_t
        sin_port        dw      ?       ; in_port_t
        sin_addr        dd      ?       ; struct in_addr
        sin_zero        rb      8       ; zero
ends

struct CONNECT_DATA ; 16*4 = 64 bytes
        socket          dd 0 ; connection socket number
        sockaddr        sockaddr_in ; socaddr connection
        buffer_request  dd 0 ; pointer to buffer for geting message socket
        request_size    dd 0 ; size geted data from client
        end_buffer_request dd 0 ; privat data for parser
        buffer_response dd 0 ; pointer to buffwr for resp message 
        http_method     dd 0 ; pointer to string
        http_verion     dd 0 ; pointer to the asciiz string with
                             ; protocol version
        num_headers     dd 0 ; number items in REQUEST_DATA
        http_headers    dd 0 ; pointer to array REQUEST_DATA of HTTP headers
        uri_scheme      dd 0 ; pointer to string
        uri_authority   dd 0 ; pointer to string(%XX not converted)
        uri_path        dd 0 ; pointer to converted uri path in UTF-8 string
        num_uri_args    dd 0 ;
        uri_arg         dd 0 ; pointer to the REQUEST_DATA array of string
                             ; uri arguments
        uri_fragment    dd 0 ; pointer to string
        message_body    dd 0 ; pointer to body of http request
        message_body_len dd 0 ; length message_body in buffer
ends

diff10 'sizeof.CONNECT_DATA: ', 0, sizeof.CONNECT_DATA

struct FILED
        opcode          rd 1
        offset          rd 2
        size            rd 1
        buffer          rd 1
        name_encode     rd 1
        path            rd 1
        end_path        rd 1
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
        Alloc           rd 1
        Free            rd 1
        parse_http_query rd 1 ; not standart calling, not using this function
        ;---------------------------------------------------------------------
        FileInitFILED   rd 1
        ;void stdcall FileInitFILED(FILED* buffer, char* path);
        FileInfo        rd 1
        ;FS_STATUS stdcall FileInfo(char* path, void* buffer);
        FileRead        rd 1
        ;uint32_t stdcall FileRead(FILED* file, void* buff, uint32_t size);
        FileSetOffset   rd 1
        ;void stdcall FileSetOffset(FILED* file, uint64_t offset);
        FileReadOfName  rd 1
        ;uint32_t FileReadOfName(char* path, void* buff, uint32_t size);
        ;---------------------------------------------------------------------
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
        ; uint32_t add_http_header(RESPD* ptr, char* header, uint32_t length);
        del_http_header rd 1
        ; uint32_t del_http_header(RESPD* ptr, char* header);
        ;   no del std header
        set_http_ver    rd 1
        ; void set_http_ver(RESPD* ptr, char* version, uint32_t length);
        ;   example: 'RTSP/1.1'
        begin_send_resp rd 1
        ;uint32_t begin_send_resp(RESPD* ptr, uint64_t content_length);
        finish_send_resp rd 1
        ;uint32_t finish_send_resp(RESPD* ptr);
        ;---------------------------------------------------------------------
        find_uri_arg    rd 1
        ;char* find_uri_arg(CONNECT_DATA* session, char* key);
        find_header     rd 1
        ;char* find_header(CONNECT_DATA* session, char* key);
        read_http_body  rd 1
        ;uint32_t read_http_body(CONNECT_DATA* session, char* buff, uint32_t len)
        get_mime_type   rd 1
        ;char* stdcall Get_MIME_Type(FILED* fd); //path is ASCIIZ string
        close_server    rd 1
        ;void close_server();

        GLOBAL_DATA     rd 1
ends

HTTPD_THREAD_STACK = 0x4000     ; 16 kib
