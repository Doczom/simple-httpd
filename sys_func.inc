; ABSTRACT SYSTEM FUNCTIONS

;======== inclede network.inc ========
; Socket types
SOCK_STREAM     = 1
SOCK_DGRAM      = 2
SOCK_RAW        = 3

; IP protocols
IPPROTO_IP      = 0
IPPROTO_ICMP    = 1
IPPROTO_TCP     = 6
IPPROTO_UDP     = 17
IPPROTO_RAW     = 255

; IP options
IP_TTL          = 2

; Address families
AF_UNSPEC       = 0
AF_LOCAL        = 1
AF_INET4        = 2     ; IPv4
AF_INET6        = 10    ; IPv6

PF_UNSPEC       = AF_UNSPEC
PF_LOCAL        = AF_LOCAL
PF_INET4        = AF_INET4
PF_INET6        = AF_INET6

; Flags for addrinfo
AI_PASSIVE      = 1
AI_CANONNAME    = 2
AI_NUMERICHOST  = 4
AI_NUMERICSERV  = 8
AI_ADDRCONFIG   = 0x400

; internal definition
AI_SUPPORTED    = 0x40F

; for system function 76
API_ETH         = 0 shl 16
API_IPv4        = 1 shl 16
API_ICMP        = 2 shl 16
API_UDP         = 3 shl 16
API_TCP         = 4 shl 16
API_ARP         = 5 shl 16
API_PPPOE       = 6 shl 16

; Socket flags for user calls
MSG_PEEK        = 0x02
MSG_DONTWAIT    = 0x40

; Socket levels
SOL_SOCKET      = 0xffff

; Socket options
SO_BINDTODEVICE = 1 shl 9
SO_NONBLOCK     = 1 shl 31

struct addrinfo
        ai_flags        dd      ?       ; bitmask of AI_*
        ai_family       dd      ?       ; PF_*
        ai_socktype     dd      ?       ; SOCK_*
        ai_protocol     dd      ?       ; 0 or IPPROTO_*
        ai_addrlen      dd      ?       ; length of ai_addr
        ai_canonname    dd      ?       ; char*
        ai_addr         dd      ?       ; struct sockaddr*
        ai_next         dd      ?       ; struct addrinfo*
ends

EAI_ADDRFAMILY  = 1
EAI_AGAIN       = 2
EAI_BADFLAGS    = 3
EAI_FAIL        = 4
EAI_FAMILY      = 5
EAI_MEMORY      = 6
EAI_NONAME      = 8
EAI_SERVICE     = 9
EAI_SOCKTYPE    = 10
EAI_BADHINTS    = 12
EAI_PROTOCOL    = 13
EAI_OVERFLOW    = 14

; Socket error codes
; Error Codes
ENOBUFS                 = 1
EINPROGRESS             = 2
EOPNOTSUPP              = 4
EWOULDBLOCK             = 6
ENOTCONN                = 9
EALREADY                = 10
EINVAL                  = 11
EMSGSIZE                = 12
ENOMEM                  = 18
EADDRINUSE              = 20
ECONNREFUSED            = 61
ECONNRESET              = 52
EISCONN                 = 56
ETIMEDOUT               = 60
ECONNABORTED            = 53
;======== End include========

; stdcall socket(uint32_t domain, type, proto)
netfunc_socket:
        push    ebx esi
        mcall   SF_NETWORK_SOCKET, SSF_OPEN,\
                        [esp + 2*4 + 4], [esp + 2*4 + 8], [esp + 2*4 + 12]
        ;mov     [fs:0], ebx ;errno
        pop     esi ebx
        ret     12

;stdcall close(uint32_t sock_number);
netfunc_close:
        push    ebx
        mcall    SF_NETWORK_SOCKET, SSF_CLOSE, [esp + 4 + 4]
        pop     ebx
        ret     4

;stdcall bind(uint32_t sock_num, sockaddr* _sockaddr_struct, uint32_t sockaddr_size)
netfunc_bind:
        push    esi ebx
        mcall   SF_NETWORK_SOCKET, SSF_BIND,\
                        [esp + 2*4 + 4], [esp + 2*4 + 8], [esp + 2*4 + 12]
        pop     ebx esi
        ret     12

;stdcall listen(uint32_t socket, uint32_t backlog)
netfunc_listen:
        push    ebx
        mcall   SF_NETWORK_SOCKET, SSF_LISTEN,\
                        [esp + 4 + 4], [esp + 4 + 8]
        pop     ebx
        ret     8
;stdcall accept(uint32_t socket, sockaddr* new_sockaddr_struct, uint32_t sockaddr_size)
netfunc_accept:
        push    esi ebx
        mcall   SF_NETWORK_SOCKET, SSF_ACCEPT,\
                        [esp + 2*4 + 4], [esp + 2*4 + 8], [esp + 2*4 + 12]
        pop     ebx esi
        ret     12
;stdcall send(uint32_t socket, void* buff, uint32_t len_buff, uint32_t flags)
netfunc_send:
        push    esi edi ebx
        mcall   SF_NETWORK_SOCKET, SSF_SEND,\
                        [esp + 3*4 + 4], [esp + 3*4 + 8],\
                        [esp + 3*4 + 12], [esp + 3*4 + 16]
        pop     ebx edi esi
        ret     16
;stdcall recv(uint32_t socket, void* buff, uint32_t len_buff, uint32_t flags)
netfunc_recv:
        push    esi edi ebx
        mcall   SF_NETWORK_SOCKET, SSF_RECEIVE,\
                        [esp + 3*4 + 4], [esp + 3*4 + 8],\
                        [esp + 3*4 + 12], [esp + 3*4 + 16]
        pop     ebx edi esi
        ret     16

macro   ThreadExit {
        mcall   SF_TERMINATE_PROCESS
}

; stdcall CreatThread(void* entry_thread);
CreateThread:
        push    ebx edi
        ;alloc memory for stack
        mcall   SF_SYS_MISC, SSF_MEM_ALLOC, HTTPD_THREAD_STACK
        test    eax, eax
        jz      .err

        mov     ecx, HTTPD_THREAD_STACK/4
        mov     edi, eax
        ;start thread for new connection
        mov     edx, eax
        xor     eax, eax
        rep stosd
        add     edx, HTTPD_THREAD_STACK - 4
        m2m     dword[edx], dword[esp + 2*4 + 4]
        mcall   SF_CREATE_THREAD, 1, .thread ;<- thread entry
.err:
        pop     ebx edi
        ret     4

.thread:
        pop     eax
        call    eax
        lea     ecx, [esp - HTTPD_THREAD_STACK]
        mcall   SF_SYS_MISC, SSF_MEM_FREE
        ThreadExit


InitHeap:
        push    ebx
        mcall   SF_SYS_MISC, SSF_HEAP_INIT  ; init heap
        pop     ebx
        ret

; stdcall Alloc(uint32_t size)
Alloc:
        push    ebx
        mcall   SF_SYS_MISC, SSF_MEM_ALLOC, [esp + 4 + 4]
        pop     ebx
        ret     4

; stdcall Free(void* ptr)
Free:
        push    ebx
        mcall   SF_SYS_MISC, SSF_MEM_FREE, [esp + 4 + 4]
        pop     ebx
        ret     4

;-----------------------------------------------------------------------------

;FS_STATUS stdcall FileInfo(char* path, void* buffer)
FileInfo:
        mov     ecx, [esp + 4]
        mov     edx, [esp + 8]
        push    ebx
        sub     esp, sizeof.FILED
        mov     dword[esp + FILED.opcode], SSF_GET_INFO ; file info
        mov     dword[esp + FILED.offset + 4], 0 ; zero flag
        mov     dword[esp + FILED.buffer], edx
        mov     dword[esp + FILED.name_encode], 3 ;UTF-8
        mov     dword[esp + FILED.path], ecx
        mov     ebx, esp
        mcall   80
        add     esp, sizeof.FILED
        pop     ebx
        ret     8

; void stdcall FileInitFILED(void* buffer, char* path)
FileInitFILED:
        push    ecx edi
        mov     edi, [esp + 4*2 + 4]
        mov     ecx, sizeof.FILED/4
        xor     eax, eax
        rep stosd
        mov     eax, [esp + 4*2 + 4]
        mov     ecx, [esp + 4*2 + 8]
        mov     [eax + FILED.path], ecx
        mov     dword[eax + FILED.name_encode], 3 ;UTF-8
        dec     ecx
@@:
        inc     ecx
        cmp     byte[ecx], 0
        jne     @b

        mov     [eax + FILED.end_path], ecx
        pop     edi ecx
        ret     8

;uint32_t stdcall FileRead(FILED* file, void* buffer, uint32_t size)
FileRead:

        mov     eax, [esp + 4]
        mov     ecx, [esp + 8]
        mov     edx, [esp + 12]

        push    ebx
        mov     [eax + FILED.opcode], SSF_READ_FILE
        mov     [eax + FILED.size], edx
        mov     [eax + FILED.buffer], ecx

        mov     ebx, eax
        mcall   80

        test    eax, eax
        jz      @f
        cmp     eax, FSERR_END_OF_FILE ; EOF
        je      @f
        xor     ebx, ebx
@@:
        mov     eax, ebx

        pop     ebx
        mov     ecx, [esp + 4]
        add     [ecx + FILED.offset], eax
        adc     [ecx + FILED.offset], 0

        ret     12


;void stdcall FileSetOffset(FILED* file, uint64_t offset)
FileSetOffset:
        push    ecx
        mov     eax, [esp + 4]
        mov     ecx, [esp + 8]
        mov     [eax + FILED.offset], ecx
        mov     ecx, [esp + 12]
        mov     [eax + FILED.offset + 4], ecx
        pop     ecx
        ret     12

;uint32_t stdcall FileReadOfName(char* path, void* buffer, uint32_t size)
FileReadOfName:
        mov     ecx, [esp + 4]
        mov     edx, [esp + 8]
        mov     eax, [esp + 12]
        push    ebx
        sub     esp, sizeof.FILED
        mov     dword[esp + FILED.opcode], SSF_READ_FILE
        mov     dword[esp + FILED.offset], 0
        mov     dword[esp + FILED.offset + 4], 0
        mov     dword[esp + FILED.size], eax
        mov     dword[esp + FILED.buffer], edx
        mov     dword[esp + FILED.name_encode], 3 ;UTF-8
        mov     dword[esp + FILED.path], ecx
        mov     ebx, esp
        mcall   80
        test    eax, eax
        jz      @f
        cmp     eax, FSERR_END_OF_FILE ; EOF
        je      @f
        xor     ebx, ebx
@@:
        mov     eax, ebx
        add     esp, sizeof.FILED
        pop     ebx
        ret     12



;-----------------------------------------------------------------------------


;RESPD* stdcall create_resp(CONNECT_DATA* session, uint32_t flags)
create_resp:
        stdcall Alloc, sizeof.RESPD
        test    eax, eax
        jz      .exit
        
        mov     edx, [esp + 4]
        mov     ecx, [esp + 8]
        mov     [eax + RESPD.flags], ecx
        mov     [eax + RESPD.session], edx
        ; set default values 
        mov     [eax + RESPD.http_status], '200 '
        mov     [eax + RESPD.http_ver_ptr], default_http_version
        mov     [eax + RESPD.http_ver_len], default_http_version.length
        mov     [eax + RESPD.count_header], 0 
.exit:
        ret     8

;void stdcall destruct_resp(RESPD* ptr)
destruct_resp:
        stdcall Free, [esp + 4]
        ret     4

;void set_http_status(RESPD* ptr, uint32_t status) // status in '200' format
set_http_status:
        mov     eax, [esp + 4]
        mov     ecx, [esp + 8]
        and     ecx, 0x00ffffff ; clear 3 byte of register
        add     ecx, 0x20 shl 24 ; set 3 byte in ' '
        mov     [eax + RESPD.http_status], ecx
        ret     8
;void set_http_ver(RESPD* ptr, char* version, uint32_t length) // example: 'RTSP/1.1 '
set_http_ver:
        mov     eax, [esp + 4]
        mov     ecx, [esp + 8]
        mov     edx, [esp + 12]
        mov     [eax + RESPD.http_ver_ptr], ecx
        mov     [eax + RESPD.http_ver_len], edx

        ret     12

;uint32_t add_http_header(RESPD* ptr, char* ptr_header, uint32_t length)
add_http_header:
        mov     eax, [esp + 4]
        mov     ecx, [eax + RESPD.count_header]
        cmp     ecx, 64
        jz      .err
        
        inc     dword[eax + RESPD.count_header]
        mov     edx, [esp + 8]
        mov     [eax + ecx*8 + RESPD.header.ptr], edx
        mov     edx, [esp + 12]
        mov     [eax + ecx*8 + RESPD.header.len], edx
        mov     eax, ecx
        ret     12
.err:
        mov     eax, -1
        ret     12


;uint32_t del_http_header(RESPD* ptr, char* ptr_header) // no del std header
del_http_header:
        mov     eax, [esp + 4]
        mov     edx, [esp + 8]
        mov     ecx, [eax + RESPD.count_header]
        add     eax, RESPD.header.ptr
@@:
        test    ecx, ecx 
        jz      .err

        cmp     [eax], edx
        je      .found

        add     eax, 8
        dec     ecx
        jmp     @b
.found:
        dec     ecx ; skiip one item
@@:
        test    ecx, ecx
        jz      .exit
        mov     edx, [eax + 8]
        mov     [eax], edx
        mov     edx, [eax + 8 + 4]
        mov     [eax + 4], edx

        add     eax, 8
        jmp     @b
.exit:
        mov     ecx, [esp + 4]
        dec     dword[ecx + RESPD.count_header]
        ret     8
.err:
        mov     eax, -1
        ret     8


;uint32_t send_resp(RESPD* ptr, char* content, uint32_t length)
send_resp:
        push    esi edi ebp
        mov     ebp, [esp + 4*3 + 4]

        test    [ebp + RESPD.flags], FLAG_TRANSFER_CHUNKED
        jnz     .chunked

        test    [ebp + RESPD.flags], FLAG_RAW_STREAM
        jnz     .send_content

        mov     edx, [esp + 4*3 + 12]
        xor     ecx, ecx

        push    ecx
        push    edx
        push    ebp
        call    begin_send_resp

        cmp     eax, -1
        jz      .exit
        ; send content 
.send_content:
        mov     ecx, [esp + 4*3 + 8] ; ptr
        test    ecx, ecx
        jz      .exit
        
        mov     eax, [ebp + RESPD.session]
        push    dword 0
        push    dword[esp + 4*4 + 12] ; size
        push    ecx
        push    dword[eax + CONNECT_DATA.socket]
        call    netfunc_send
.exit:
        pop     ebp edi esi
        ret     12


.chunked:
        mov     eax, -1
        mov     ecx, [esp + 4*3 + 8] ; ptr
        test    ecx, ecx
        jz      .exit
        mov     ecx, [esp + 4*3 + 12] ; size
        test    ecx, ecx
        jz      .exit

        ; send db <HEX size>, 13, 10
        mov     eax, [esp + 4*3 + 12]
        push    word 0x0a0d
        sub     esp, 8
        lea     esi, [esp + 7]
        mov     ecx, 8
@@:
        mov     edx, eax
        and     edx, 1111b
        mov     dl, byte[hex_chars + edx]
        mov     [esi], dl
        shr     eax, 4
        dec     esi
        loop    @b

        mov     ecx, esp
        mov     eax, [ebp + RESPD.session]
        push    dword 0
        push    10 ; size
        push    ecx
        push    dword[eax + CONNECT_DATA.socket]
        call    netfunc_send
        add     esp, 2+8

        ;cmp     eax, -1
        ;jz      .exit

        ; send content
        mov     ecx, [esp + 4*3 + 8] ; ptr
        
        mov     eax, [ebp + RESPD.session]
        push    dword 0
        push    dword[esp + 4*4 + 12] ; size
        push    ecx
        push    dword[eax + CONNECT_DATA.socket]
        call    netfunc_send

        ;cmp     eax, -1
        ;jz      .exit

        ; send db 13, 10
        push    word 0x0a0d
        mov     ecx, esp ; ptr
        
        mov     eax, [ebp + RESPD.session]
        push    dword 0
        push    2 ; size
        push    ecx
        push    dword[eax + CONNECT_DATA.socket]
        call    netfunc_send

        add     esp, 2
        pop     ebp edi esi
        ret     12

;char* find_uri_arg(CONNECT_DATA* session, char* key)
find_uri_arg:
        push    esi edi
        mov     edx, [esp + 4*2 + 4]
        mov     ecx, [edx + CONNECT_DATA.num_uri_args]
        mov     esi, [esp + 4*2 + 8]
        mov     edx, [edx + CONNECT_DATA.uri_arg]
        test    ecx, ecx
        jz      .not_found
.loop:
        mov     edi, [edx]
        push    esi
@@:
        cmpsb
        jne     @f
        cmp     byte[esi - 1], 0
        jne     @b

        pop     esi
        mov     eax, [edx + 4]
        jmp     .exit
@@:
        pop     esi
        add     edx, 4*2 ; size array item
        loop    .loop
.not_found:
        xor     eax, eax
.exit:
        pop     edi esi
        ret     8

;char* find_header(CONNECT_DATA* session, char* key)
find_header:
        push    esi edi
        mov     edx, [esp + 4*2 + 4]
        mov     ecx, [edx + CONNECT_DATA.num_headers]
        mov     esi, [esp + 4*2 + 8]
        mov     edx, [edx + CONNECT_DATA.http_headers]
        test    ecx, ecx
        jz      .not_found
.loop:
        mov     edi, [edx]
        push    esi
@@:
        cmpsb
        jne     @f
        cmp     byte[esi - 1], 0
        jne     @b

        pop     esi
        mov     eax, [edx + 4]
        jmp     .exit
@@:
        pop     esi
        add     edx, 4*2 ; size array item
        loop    .loop
.not_found:
        xor     eax, eax
.exit:
        pop     edi esi
        ret     8

;uint32_t begin_send_resp(RESPD* ptr, uint64_t content_length);
begin_send_resp:
        ; send status line, headers and \n
        push    esi edi ebp
        ; get full size 
        mov     ebp, [esp + 4*3 + 4]
        mov     ecx, [ebp + RESPD.count_header]
        shl     ecx, 1 ; *2
        add     ecx, 2 + 5 + 2 ; finish 0x0d 0x0a + status(5 byte) + 0x0d 0x0a
        add     ecx, [ebp + RESPD.http_ver_len]
        ;       add size all headers
        xor     edx, edx
@@:
        cmp     edx, [ebp + RESPD.count_header]
        je      @f
        add     ecx, [ebp + edx*8 + RESPD.header.len]
        inc     edx
        jmp     @b
@@:
        ; add size  default headers
        ;add     ecx, response.end_headers - response.code
        test    [ebp + RESPD.flags], FLAG_NO_CONNECTION
        jnz     @f
        add     ecx, default_http_connection.length
@@:
        test    [ebp + RESPD.flags], FLAG_NO_SERVER_HEADER
        jnz     @f
        add     ecx, default_http_header_server.length
@@:
        test    [ebp + RESPD.flags], FLAG_NO_CONTENT_ENCODING
        jnz     @f
        add     ecx, default_http_cont_encod.length
@@:
        test    [ebp + RESPD.flags], FLAG_TRANSFER_CHUNKED
        jz      @f
        add     ecx, http_header_transfer_chunked.length
@@:
        test    [ebp + RESPD.flags], FLAG_NO_CONTENT_LENGTH
        jnz     @f
        add     ecx, default_http_cont_len.length
@@:
        test    [ebp + RESPD.flags], FLAG_NO_CONTENT_TYPE
        jnz     @f
        add     ecx, default_http_cont_type.length
@@:
        test    [ebp + RESPD.flags], FLAG_NO_CACHE_CONTROL
        jnz     @f
        add     ecx, default_http_cache_ctl.length
@@:
        mov     [ebp + RESPD.buffer_size], ecx
        ; alloc buffer
        stdcall Alloc, ecx
        test    eax, eax 
        jz      .error

        mov     [ebp + RESPD.buffer], eax
        mov     edi, eax
        ; copy data - version, status code, default header
        mov     ecx, [ebp + RESPD.http_ver_len]
        mov     esi, [ebp + RESPD.http_ver_ptr]
        rep movsb ; copy http ver

        mov     al, ' '
        stosb
        mov     eax, [ebp + RESPD.http_status]
        stosd
        mov     eax, 0x0a0d
        stosw

        ; copy and creating default headers
        test    [ebp + RESPD.flags], FLAG_NO_SERVER_HEADER
        jnz     @f
        mov     esi, default_http_header_server
        mov     ecx, default_http_header_server.length
        rep movsb
@@:
        test    [ebp + RESPD.flags], FLAG_NO_CACHE_CONTROL
        jnz     @f
        mov     esi, default_http_cache_ctl
        mov     ecx, default_http_cache_ctl.length
        rep movsb
@@:
        test    [ebp + RESPD.flags], FLAG_NO_CONTENT_ENCODING
        jnz     @f
        mov     esi, default_http_cont_encod
        mov     ecx, default_http_cont_encod.length
        rep movsb
@@:
        test    [ebp + RESPD.flags], FLAG_TRANSFER_CHUNKED
        jz      @f

        mov     esi, http_header_transfer_chunked
        mov     ecx, http_header_transfer_chunked.length
        rep movsb
@@:
        test    [ebp + RESPD.flags], FLAG_NO_CONTENT_LENGTH
        jnz     .no_content_length

        mov     esi, default_http_cont_len
        mov     ecx, default_http_cont_len.length
        rep movsb
        ; set content length
        mov     eax, [esp + 4*3 + 8]
        mov     edx, [esp + 4*3 + 12]

        test    eax, eax
        jne     @f
        test    edx, edx
        jz      .no_content_length
@@:
        cmp     edx, 1000 ; 999*4 - max length of file
        jae     .error_free

        lea     ecx, [edi - 1 - 2] ; skip 13, 10 and set on finish char number

.div1000:
        div     dword[_DIV_1000_]
        push    eax

        mov     eax, edx
        xor     edx, edx

        div     dword[_DIV_100_]
        push    eax
        
        mov     eax, edx
        xor     edx, edx
        div     dword[_DIV_10_]
        add     byte[ecx], dl
        add     byte[ecx - 1], al
        sub     ecx, 3

        pop     eax
        add     byte[ecx + 1], al

        pop     eax
        xor     edx, edx
        test    eax, eax
        jne     .div1000

.no_content_length:
        test     [ebp + RESPD.flags], FLAG_NO_CONTENT_TYPE
        jnz      @f

        mov      esi, default_http_cont_type
        mov      ecx, default_http_cont_type.length
        rep movsb
@@:
        test     [ebp + RESPD.flags], FLAG_NO_CONNECTION
        jnz      @f

        mov     esi, default_http_connection
        mov     ecx, default_http_connection.length
        rep movsb

        test    [ebp + RESPD.flags], FLAG_KEEP_ALIVE
        jz      @f

        lea     ecx, [edi - default_http_connection.length \
                          + default_http_connection.value]
        mov     dword[ecx], 'keep'
        mov     dword[ecx + 4], '-ali'
        mov     word[ecx + 8], 've'
@@:
        ; copy addition headers
        lea     eax, [ebp + RESPD.header.ptr]
        xor     edx, edx 
@@:
        cmp     edx, [ebp + RESPD.count_header]
        je      @f

        mov     esi, [eax]
        mov     ecx, [eax + 4]
        rep movsb

        mov     word[edi], 0x0a0d
        add     edi, 2

        add     eax, 8
        inc     edx
        jmp     @b
@@:
        mov     ax, 0x0A0D
        stosw

        ; send response status line and headers
        mov     eax, [ebp + RESPD.session]

        push    dword 0
        push    dword[ebp + RESPD.buffer_size]
        push    dword[ebp + RESPD.buffer]
        push    dword[eax + CONNECT_DATA.socket]
        call    netfunc_send

        push    eax
        stdcall Free, [ebp + RESPD.buffer]
        pop     eax
        pop     ebp edi esi
        ret     12

.error_free:
        stdcall Free, [ebp + RESPD.buffer]
.error:
        mov     eax, -1
        pop     ebp edi esi
        ret     12


;uint32_t finish_send_resp(RESPD* ptr);
finish_send_resp:
        ; send finish block for chunked encoding
        mov     edx, [esp + 4]
        test    dword[edx + RESPD.flags], FLAG_TRANSFER_CHUNKED
        jz      .exit
        ; send finish block - zero length
        sub     esp, 5
        mov     dword[esp + 1], 0x0a0d0a0d
        mov     byte[esp], '0'

        mov     ecx, esp
        mov     eax, [edx + RESPD.session]

        push    dword 0
        push    dword 5 ; size buffer
        push    ecx
        push    dword[eax + CONNECT_DATA.socket]
        call    netfunc_send

        add     esp, 5
.exit:
        ret     4

; uint32_t read_http_body(CONNECT_DATA* session, char* buff, uint32_t len)
read_http_body:
        push    esi edi
        ; check length in buffer
        xor     eax, eax
        mov     edx, [esp + 4*2 + 4]
        mov     ecx, [esp + 4*2 + 12]
        mov     edi, [esp + 4*2 + 8]
        cmp     dword[edx + CONNECT_DATA.message_body_len], 0
        jz      .get_new_buffer
.local_buffer:
        mov     esi, [edx + CONNECT_DATA.message_body]
        cmp     ecx, [edx + CONNECT_DATA.message_body_len]
        pushfd
        jbe     @f
        mov     ecx, [edx + CONNECT_DATA.message_body_len]
@@:
        sub     [edx + CONNECT_DATA.message_body_len], ecx
        add     [edx + CONNECT_DATA.message_body], ecx
        push    ecx
        rep movsb
        pop     eax

        popfd
        jbe     .exit
        sub     ecx, eax
        ; ecx - not readed bytes
        ; eax - count reading bytes
.get_new_buffer:
        push    eax

        ;read data from socket
        push    dword 0 ;flags
        push    ecx
        push    esi
        push    dword[edx + CONNECT_DATA.socket]
        call    netfunc_recv

        cmp     eax, -1
        je      .exit

        add     eax, [esp]
        add     esp, 4
.exit:
        pop     edi esi
        ret     12


;-----------------------------------------------------------------------------
;void close_server()
; TODO: added free HTTPD_MODULE structures
close_server:
        ; call function httpd_close() for all modules
        ; terminate main thread server
        mov     dword[srv_stop], 1

        cmp     dword[GLOBAL_DATA.modules], 0
        jz      .no_modules

        mov     eax, [GLOBAL_DATA.modules]
.next_module:
        push    eax
        push    dword[eax + HTTPD_MODULE.pdata] ; context of module
        call    dword[eax + HTTPD_MODULE.httpd_close]
        pop     eax

        mov     eax, [eax] ; HTTPD_MODULE.next
        test    eax, eax ; terminate list
        jne     .next_module

.no_modules:
        mov     dword[srv_shutdown], 1
        ret