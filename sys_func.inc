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