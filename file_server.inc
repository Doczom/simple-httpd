; This is a module for processing standard requests to get a file along 
; a path that does not belong to another module.

SIZE_FILE_BUFFER        = 64*1024

file_server:

        ; check http version (skip RTSP)
        mov     ecx, [esi + CONNECT_DATA.http_verion]
        cmp     dword[ecx], 'HTTP'
        jne     .err_http_501
        cmp     word[ecx + 4], '/1' 
        jne     .err_http_501
        
;        cmp     byte[ecx + 7], '1'
;        je      .http_1_1 
;        ; http 1.0;
;
;.http_1_1: 

        ; check name on ../../ and other bad items
        mov     ecx, [esi + CONNECT_DATA.uri_path]
@@:
        cmp     byte[ecx], 0
        je      .good_name
        inc     ecx
        cmp     dword[ecx - 1], '/../' ; if path "/.." then system read dir and 
        jne     @b                     ; and skip this path with 404 error
        jmp     .err_http_501

.good_name:

        ; check file of name (generate full name: work_dir + path)
        ;
        ; alloc 4 kib for path to file name 
        ; memory alloced on stack and for working program alloced with buffer 
        ; for code stack 
        lea     edx, [esp - 6*1024 - 4096] ; edx <- buffer for full path
        
        mov     edi, edx
        sub     edi, sizeof.FILED; edi <- struct FILED 
        mov     [edi + FILED.path], edx
        
        mov     eax, esi
        push    esi edi
        ;; copy work_dir
        mov     esi, GLOBAL_DATA.work_dir
        mov     edi, edx
        mov     ecx, [GLOBAL_DATA.work_dir.size]
        rep movsb 
        ;; copy path 
        mov     esi, [eax + CONNECT_DATA.uri_path]
@@: 
        movsb
        cmp     byte[esi - 1], 0
        jne     @b

        mov     eax, edi
        dec     eax
        pop     edi esi
        mov     [edi + FILED.end_path], eax

        ;; check file info (skip of directory and if syscall return error)
        lea     eax, [edi - 40] ; buffer for file info
        mov     [edi + FILED.buffer], eax

        push    edi
        call    FileInfo
        test    eax, eax 
        jnz     .err_not_found

        test    dword[edi - 40], 11000b ; check dir OR disk partition
        jnz     .err_not_found


        ; check method OPTIONS
        mov     ecx, [esi + CONNECT_DATA.http_method]
        cmp     dword[ecx], 'OPTI'
        jne     @f
        cmp     dword[ecx + 4], 'ONS'
        je      .send_options
@@:
        cmp     dword[ecx], 'POST'
        jne     @f
        cmp     byte[ecx + 4], 0
        je      .send_file
@@:
        cmp     dword[ecx], 'GET'
        jne     .err_http_501

.send_file:
        ; create http response (set 200 code, MINE type and length of body)
        ;; alloc 33 kib 
        push    dword SIZE_FILE_BUFFER
        call    Alloc
        test    eax, eax
        jz      .err_http_501 ; error memory

        push    esi
        push    edi
        push    eax ; save pointer for Free

        mov     esi, base_response
        mov     ecx, response.body
        mov     edi, [esp]
        rep movsb

        mov     edi, [esp]
        mov     dword[edi + response.code], '200 '
        
        mov     dword[edi + response.connection], 'keep'
        mov     dword[edi + response.connection + 4], '-ali'
        mov     word[edi + response.connection + 8], 've'
        
        
        push    dword[esp + 4] ; <-FILED
        call    Get_MIME_Type

        add     edi, response.content_type
        mov     esi, eax
@@:
        movsb
        cmp     byte[esi], 0
        jne     @b
        
        
        pop     edi ; buffer on 32 kib
        pop     ebx ; FD
        pop     esi ; CONNECT_DATA
        
        ; copy length
        mov     edx, [ebx - 40 + 36]
        mov     eax, [ebx - 40 + 32]
        lea     ecx, [edi + response.content_len + 21]
        test    eax, eax 
        jne     @f
        test    edx, edx
        jz      .err_not_found_1
@@:
        cmp     edx, 1000 ; 999*4 - max length of file
        jae     .err_not_found_1
        ; edx:eax  ecx - end char in string '000000'
.div1000:
        div     dword[_DIV_1000_]
        push    eax

        mov     eax, edx
        xor     edx, edx
;.div100:
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
        ;xor     edx, edx
        ;test    eax, eax
        ;jne     .div100
        pop     eax
        xor     edx, edx
        test    eax, eax
        jne     .div1000

        ;send_response_header 
        push    dword 0 ; flags
        push    response.body
        push    edi
        push    dword[esi + CONNECT_DATA.socket]
        call    netfunc_send
        cmp     eax, -1
        je      .exit_free

        mov     dword[ebx + FILED.offset], 0
        mov     dword[ebx + FILED.offset + 4], 0
        mov     [ebx + FILED.buffer], edi
        mov     [ebx + FILED.size], SIZE_FILE_BUFFER
.send_response_body:
        ; read 32 kib to file 
        push    ebx
        call    FileRead
        test    eax, eax
        jz      @f
        cmp     eax, 6

        mov     [0], eax
        jne     .exit_free
@@:
        ; send this block data 
        push    dword 0 ; flags
        push    SIZE_FILE_BUFFER
        push    edi
        push    dword[esi + CONNECT_DATA.socket]
        call    netfunc_send
        cmp     eax, -1
        je      .exit_free

        ; check end file
        mov     ecx, [ebx + FILED.offset + 4]
        cmp     [ebx - 4], ecx
        ja      .add_offset

        mov     edx, [ebx + FILED.offset]
        cmp     [ebx - 8], edx
        jbe     .exit_free    

.add_offset:
        add     [ebx + FILED.offset], SIZE_FILE_BUFFER
        adc     [ebx + FILED.offset + 4], 0

        jmp     @b

.exit_free:
        push    edi
        call    Free
        ret

.send_options:
        ; send standart options of file server
        push    dword 0 ; flags
        push    http_response_options.size
        push    http_response_options
        push    dword[esi + CONNECT_DATA.socket]
        call    netfunc_send

        ret

.err_not_found_1:
        push    edi
        call    Free
.err_not_found:
        ; send error 404
        push    dword 0 ; flags
        push    http_response_err_404.size
        push    http_response_err_404
        push    dword[esi + CONNECT_DATA.socket]
        call    netfunc_send

        ret

.err_http_501:
        ; send error 501
        push    dword 0 ; flags
        push    http_response_err_501.size
        push    http_response_err_501
        push    dword[esi + CONNECT_DATA.socket]
        call    netfunc_send

        ret


; char* stdcall Get_MIME_Type(FILED* fd); //path is ASCIIZ string
Get_MIME_Type:
        push    esi edi
        mov     eax, [esp + 4*2 + 4]
        mov     edx, [eax + FILED.path]
        mov     eax, [eax + FILED.end_path]
        push    eax
        sub     [esp], edx
        mov     edx, [GLOBAL_DATA.MIME_types_arr]
@@:
        mov     esi, [edx]
        add     edx, 4
        cmp     dword[esi], 0
        jz      .other
        
        mov     edi, eax
        movzx   ecx, byte [esi]
        cmp     ecx, [esp]
        jae     @b

        inc     esi
        sub     edi, ecx
        repe cmpsb
        jne     @b
@@:
        mov     eax, esi
        pop     edi ;clear value in stack
        pop     edi esi 
        ret 4
.other:
        add     esi, 4
        jmp     @b