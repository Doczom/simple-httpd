#ifndef MODULE_API_H
#define MODULE_API_H

#define API_VERSION     0x100 // 0.1.0

#define FLAG_KEEP_ALIVE                 0x01
#define FLAG_NO_CONNECTION              0x02
#define FLAG_NO_SERVER_HEADER           0x04
#define FLAG_NO_CONTENT_ENCODING        0x08
#define FLAG_NO_DATE                    0x10 // not supported

#define FLAG_NO_CONTENT_LENGTH          0x20
#define FLAG_NO_CONTENT_TYPE            0x40
#define FLAG_NO_CACHE_CONTROL           0x80

#define FLAG_TRANSFER_CHUNKED           0x100
#define FLAG_RAW_STREAM                 0x200

typedef struct sockaddr_in {
        uint16_t sin_family;
        uint16_t sin_port;
        uint32_t sin_addr;
        uint64_t sin_zero;
}sockaddr_in;

typedef struct FILED{
        uint32_t opcode;
        uint64_t offset;
        uint32_t size;
        void*    buffer;
        uint32_t name_encode;
        char*    path;
        char*    end_path;
} FILED;

typedef struct CONNECT_DATA {
        int             socket;
        sockaddr_in     sockaddr;
        char*           buffer_request;
        int32_t         request_size;
        char*           end_buffer_request;
        char*           buffer_response;
        char*           http_method;
        char*           http_version;
        uint32_t        num_headers;
        uint32_t        http_headers; // fixme, bad type
        char*           uri_sheme;
        char*           uri_authority;
        char*           uri_path;
        uint32_t        num_uri_args;
        uint32_t        uri_arg;
        char*           uri_fragment;
        char*           message_body;
        uint32_t        message_body_len;
} CONNECT_DATA;

typedef struct RESPD {
        uint32_t N;
        uint32_t X;
        uint32_t Y;
        uint32_t Z;
}RESPD;

typedef struct IMPORT_DATA{
        uint32_t version;
        uint32_t _sizeof;
        int __stdcall (*netfunc_socket)(uint32_t domain, uint32_t type, uint32_t proto);
        void __stdcall (*netfunc_close)(int sock_number);
        int __stdcall (*netfunc_bind)(int sock_num, sockaddr_in* _sockaddr_struct, uint32_t sockaddr_size);
        int __stdcall (*netfunc_accept)(int socket, sockaddr_in* new_sockaddr_struct, uint32_t sockaddr_size);
        int __stdcall (*netfunc_listen)(int socket, uint32_t backlog);
        int __stdcall (*netfunc_recv)(int socket, void* buff, uint32_t len_buff, uint32_t flags);
        int __stdcall (*netfunc_send)(int socket, void* buff, uint32_t len_buff, uint32_t flags);
        void* __stdcall (*Alloc)(uint32_t size);
        void __stdcall (*Free)(void* ptr);
        uint32_t parse_http_query;
        //-----------------------------------------------------------------
        void __stdcall (*FileInitFILED)(FILED* buffer, char* path);
        int __stdcall (*FileInfo)(char* path, void* buffer);
        uint32_t __stdcall (*FileRead)(FILED* file, void* buff, uint32_t size);
        void __stdcall (*FileSetOffset)(FILED* file, uint64_t offset);
        uint32_t __stdcall (*FileReadOfName)(char* path, void* buff, uint32_t size);
        //-----------------------------------------------------------------
        uint32_t __stdcall (*send_resp)(RESPD* ptr, char* content, uint32_t length);
        RESPD* __stdcall (*create_resp)(CONNECT_DATA* session, uint32_t flags);
        void __stdcall (*destruct_resp)(RESPD* ptr);
        void __stdcall (*set_http_status)(RESPD* ptr, uint32_t status);
        uint32_t __stdcall (*add_http_header)(RESPD* ptr, char* header, uint32_t length);
        uint32_t __stdcall (*del_http_header)(RESPD* ptr, char* header);
        void __stdcall (*set_http_ver)(RESPD* ptr, char* version, uint32_t length);
        uint32_t __stdcall (*begin_send_resp)(RESPD* ptr, uint64_t content_length);
        uint32_t __stdcall (*finish_send_resp)(RESPD* ptr);
        //-----------------------------------------------------------------
        char* __stdcall (*find_uri_arg)(CONNECT_DATA* session, char* key);
        char* __stdcall (*find_header)(CONNECT_DATA* session, char* key);
        uint32_t __stdcall(*read_http_body)(CONNECT_DATA* session, char* buff, uint32_t len);
        const char* __stdcall (*get_default_mime_type)(FILED* fd); //path is ASCIIZ string
        void __stdcall (*close_server)();
        void* __stdcall (*load_mime_file)(char* file);
        char* __stdcall (*Get_MIME_Type)(void* mime_table, FILED* fd);
        int __stdcall (*get_matched_substr)(const char* const uri, char* pattern, substr_list* buff, uint32_t bufflen);
} IMPORT_DATA;

extern uint32_t __stdcall httpd_init(const IMPORT_DATA* const import, char* cmdline, const char * const uri_path, char* host, char* module_path);
extern void __stdcall httpd_serv(CONNECT_DATA* request_data, uint32_t pdata);
extern void __stdcall httpd_close(uint32_t pdata);

#endif