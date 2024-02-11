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

struct sockaddr_in
        sin_family      dw      ?       ; sa_family_t
        sin_port        dw      ?       ; in_port_t
        sin_addr        dd      ?       ; struct in_addr
        sin_zero        rb      8       ; zero
ends

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
        mcall   75, 0, [esp + 2*4 + 4], [esp + 2*4 + 8], [esp + 2*4 + 12]
        ;mov     [fs:0], ebx ;errno
        pop     esi ebx
        ret     12

;stdcall close(uint32_t sock_number);
netfunc_close:
        push    ebx
        mcall    75, 1, [esp + 4 + 4]
        pop     ebx
        ret     4

;stdcall bind(uint32_t sock_num, sockaddr* _sockaddr_struct, uint32_t sockaddr_size)
netfunc_bind:
        push    esi ebx
        mcall   75, 2, [esp + 2*4 + 4], [esp + 2*4 + 8], [esp + 2*4 + 12]
        pop     ebx esi
        ret     12

;stdcall listen(uint32_t socket, uint32_t backlog)
netfunc_listen:
        push    ebx
        mcall   75, 3, [esp + 4 + 4], [esp + 4 + 8]
        pop     ebx
        ret     8
;stdcall accept(uint32_t socket, sockaddr* new_sockaddr_struct, uint32_t sockaddr_size)
netfunc_accept:
        push    esi ebx
        mcall   75, 5, [esp + 2*4 + 4], [esp + 2*4 + 8], [esp + 2*4 + 12]
        pop     ebx esi
        ret     12
;stdcall send(uint32_t socket, void* buff, uint32_t len_buff, uint32_t flags)
netfunc_send:
        push    esi edi ebx
        mcall   75, 6, [esp + 3*4 + 4], [esp + 3*4 + 8],\
                       [esp + 3*4 + 12], [esp + 3*4 + 16]
        pop     ebx edi esi
        ret     16
;stdcall recv(uint32_t socket, void* buff, uint32_t len_buff, uint32_t flags)
netfunc_recv:
        push    esi edi ebx
        mcall   75, 7, [esp + 3*4 + 4], [esp + 3*4 + 8],\
                       [esp + 3*4 + 12], [esp + 3*4 + 16]
        pop     ebx edi esi
        ret     16

; stdcall CreatThread(void* entry_thread);
CreateThread:
        push    ebx edi
        mcall   68, 12, 0x4000 ;alloc memory 16 kib for stack
        test    eax, eax
        jz      .err

        mov     ecx, 0x4000/4
        mov     edi, eax
        ;start thread for new connection
        mov     edx, eax
        xor     eax, eax
        rep stosd
        add     edx, 0x4000
        mcall   51, 1, [esp + 2*4 + 4] ;<- thread entry
.err:
        pop     ebx edi
        ret     4
; stdcall Alloc(uint32_t size)
Alloc:
        push    ebx
        mcall   68, 12, [esp + 4 + 4]
        pop     ebx
        ret     4
; stdcall Free(void* ptr)
Free:
        push    ebx
        mcall   68, 13, [esp + 4 + 4]
        pop     ebx
        ret     4


struct FILED
        opcode  rd 1
        offset  rd 2
        size    rd 1
        buffer  rd 1
                rb 1
        path    rd 1
        end_path rd 1
ends

;NTSTATUS stdcall FileInfo(FILED* file)
FileInfo:
        push    ebx
        mov     ebx, [esp + 4*1 + 4]
        mov     dword[ebx], 5 ; file info
        mov     dword[ebx + FILED.offset + 4], 0 ; zero flag
        mcall   70
        pop     ebx
        ret     4
;NTSATTUS stdcall FileRead(FILED* file)
FileRead:
        push    ebx
        mov     ebx, [esp + 4*1 + 4]
        mov     dword[ebx], 0 ; read file
        mcall   70
        pop     ebx
        ret     4




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
        and     ecx, 0x00ffffff ; clear 3 byte register
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
        inc     dword[ecx + RESPD.count_header]
        ret     8
.err:
        mov     eax, -1
        ret     8


;uint32_t send_resp(RESPD* ptr, char* content, uint32_t length)
send_resp:
        push    esi edi ebp
        ; get full size 
        mov     ebp, [esp + 4*3 + 4]
        mov     ecx, [ebp + RESPD.count_header]
        shl     ecx, 1 ; *2
        add     ecx, 2 ; finish 0x0d 0x0a
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
        add     ecx, response.end_headers - response.code
        ;       add size content
        ;add     ecx, [esp + 4*3 + 12]

        ; alloc buffer
        push    ecx
        stdcall Alloc, ecx
        pop     ecx
        test    eax, eax 
        jz      .error_alloc

        mov     [ebp + RESPD.buffer], eax
        mov     [ebp + RESPD.buffer_size], ecx
        mov     edi, eax
        ; copy data
        mov     ecx, [ebp + RESPD.http_ver_len]
        mov     esi, [ebp + RESPD.http_ver_ptr]
        rep movsb ; copy http ver

        mov     esi, base_response + response.code
        mov     ecx, response.end_headers - response.code
        mov     edx, edi
        rep movsb ; copy default status code + headers

        mov     eax, [ebp + RESPD.http_status]
        mov     [edx], eax

        test    [ebp + RESPD.flags], FLAG_KEEP_ALIVE
        jz      @f

        push    edx
        add     edx, response.connection - response.code
        mov     dword[edx], 'keep'
        mov     dword[edx + 4], '-ali'
        mov     word[edx + 8], 've'
        pop     edx        
@@:     
        add     edx, response.content_len + 21 - response.code
        mov     ecx, edx
        ;       set content length
        mov     eax, [esp + 4*3 + 12] 
@@:
        xor     edx, edx
        test    eax, eax
        jz      @f
        div     dword[_DIV_10_]
        add     byte[ecx], dl
        dec     ecx
        jmp     @b
@@:
        ;       copy addition headers
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

        cmp     eax, -1
        jz      .exit
        ; send content 

        mov     ecx, [esp + 4*3 + 8] ; ptr
        test    ecx, ecx
        jz      .free
        
        mov     eax, [ebp + RESPD.session]
        push    dword 0
        push    dword[esp + 4*4 + 12] ; size
        push    ecx
        push    dword[eax + CONNECT_DATA.socket]
        call    netfunc_send

        cmp     eax, -1
        jz      .exit
.free:
        xor     eax, eax
        ; free buffer
        push    eax
        stdcall Free, [ebp + RESPD.buffer]
        pop     eax
.exit:
        pop     ebp edi esi
        ret     12

.error_alloc:
        mov     eax, -1
        jmp     .exit



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

;void close_server()
close_server:

        ret