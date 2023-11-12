; ABSTRACT SYSTEM FUNCTIONS

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
        call    75, 1, [esp + 4 + 4]
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

;NTSTATUS stdcall FileInfo(const char* path, void* buff)
FileInfo:

        ret     8
;NTSATTUS stdcall FileRead(const char* path, void* buff, uint32_t size)
FileRead:

        ret     12