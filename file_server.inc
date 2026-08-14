; This is a module for processing standard requests to get a file along
; a path that does not belong to another module.

SIZE_FILE_BUFFER        = 1400*23;64*1024

struct FILESRV_DATA
        flags           rd 1
        uri_path        rb 4096 - 4
        file_path       rb 4096 ; full path root dir
ends

; /*=@filesrv, ./
; /=@filesrv, ./index.html
; /favicon.ico=@filesrv, ./server.png
; /files/*=@filesrv, ./*/index.html
; /files/*.php=@filesrv, ./*/index.html
filesrv_init:
        xor     eax, eax
        ret     20
filesrv_close:
        ret     8
filesrv_serv:
        ret     4




FILESRV_FLAG_FILE       = 1b
FILESRV_FLAG_DIR        = 10b
FILESRV_FLAG_FPATH      = 100b  ; format path to file

file_server:

        ; check http version (skip RTSP)
        mov     ecx, [esi + CONNECT_DATA.http_verion]
        cmp     dword[ecx], 'HTTP'
        jne     .err_http_501
        cmp     dword[ecx + 4], '/1.0'
        je      @f
        
        cmp     dword[ecx + 4], '/1.1'
        jne     .err_http_501
@@:
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

        push    esi edi
        mov     eax, esi
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

        pop     edi esi
        ; init FILED
        mov     edi, edx
        sub     edi, sizeof.FILED; edi <- struct FILED 
        stdcall FileInitFILED, edi, edx

        ;; check file info (skip of directory and if syscall return error)
        lea     eax, [edi - 40] ; buffer for file info

        stdcall FileInfo, edx, eax
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
        cmp     dword[ecx], 'HEAD'
        je      ..check_zero

        cmp     dword[ecx], 'POST'
        jne     @f
..check_zero:
        cmp     byte[ecx + 4], 0
        je      .send_file
@@:
        cmp     dword[ecx], 'GET'
        jne     .err_http_501

.send_file:
        ; edi -> FILED
        ; edi-40 ->FILEINFO
        ; esi -> CONNECT_DATA
        stdcall create_resp, esi, FLAG_NO_CONTENT_TYPE\
                                + FLAG_RAW_STREAM
        test    eax, eax
        jz      .err_http_501
        mov     ebp, eax

        stdcall Alloc, SIZE_FILE_BUFFER
        test    eax, eax
        jz      .err_http_501_1

        ; Add content type header
        push    eax esi edi
        mov     esi, default_http_cont_type
        mov     edi, eax
        mov     ecx, default_http_cont_type.length
        rep movsb

        mov     dword[eax + default_http_cont_type.value], '    '
        mov     dword[eax + default_http_cont_type.value + 4], '    '
        mov     dword[eax + default_http_cont_type.value + 8], '    '
        lea     edi, [eax + default_http_cont_type.value]

        stdcall get_default_mime_type, [esp]
        mov     esi, eax
@@:
        movsb
        cmp     byte[esi], 0
        jne     @b
        pop     edi esi  ;!!!!! "pop eax" not runing

        mov     eax, [esp]
        stdcall add_http_header, ebp, eax, default_http_cont_type.length-2 ; skip \n\r
        ;cmp     eax, -1 ; error can`t running in this function(max 64 addition headers)

        stdcall begin_send_resp, ebp, [edi - 40 + 32], [edi - 40 + 36]
        ;cmp     eax, -1
        mov     ecx, [esi + CONNECT_DATA.http_method]
        cmp     dword[ecx], 'HEAD'
        je      .finish_send_file

.send_block_file:
        mov     eax, [esp] ; allocated buffer
        stdcall FileRead, edi, eax, SIZE_FILE_BUFFER
        test    eax, eax
        jz      .finish_send_file

        mov     ecx, [esp]
        stdcall send_resp, ebp, ecx, eax
        ;cmp     eax, -1
        jmp     .send_block_file

.finish_send_file:
        stdcall Free
        stdcall finish_send_resp, ebp
        stdcall destruct_resp, ebp

        ret

.send_options:
        ; send standart options of file server
        push    dword 0 ; flags
        push    http_response_options.size
        push    http_response_options
        push    dword[esi + CONNECT_DATA.socket]
        call    netfunc_send

        ret

.err_not_found:
        ; send error 404
        push    dword 0 ; flags
        push    http_response_err_404.size
        push    http_response_err_404
        push    dword[esi + CONNECT_DATA.socket]
        call    netfunc_send

        ret
.err_http_501_1:
        stdcall Free, ebp
.err_http_501:
        ; send error 501
        push    dword 0 ; flags
        push    http_response_err_501.size
        push    http_response_err_501
        push    dword[esi + CONNECT_DATA.socket]
        call    netfunc_send

        ret

;char* stdcall Get_MIME_Type(FILED* fd); //path is UTF-8 string
get_default_mime_type:
        stdcall Get_MIME_Type, [GLOBAL_DATA.MIME_types_arr], [esp + 4]
        ret     4


; char* stdcall Get_MIME_Type(void* mime_table, FILED* fd); //path is UTF-8 string
Get_MIME_Type:
        push    esi edi
        mov     eax, [esp + 4*2 + 8]
        mov     edx, [eax + FILED.path]
        mov     eax, [eax + FILED.end_path]
        push    eax
        sub     [esp], edx
        mov     edx, [esp + 4*3 + 4]
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
        ret     8
.other:
        add     esi, 4
        jmp     @b

;int get_replace_substr(char* uri, char* pattern, substr_list* buff, uint32_t bufflen);
; OUT: eax = count item or -1 for error
; struct substr_list {
;     u8*       ptr;
;     uint32_t  len;
; }
get_matched_substr:
        push    ebx ebp esi edi
        mov     ebp, [esp + 4*4 + 12] ; buff
        mov     ebx, [esp + 4*4 + 16]
        add     ebx, ebp ; ptr to end buffer


        mov     esi, [esp + 4*4 + 4] ; uri
        mov     edi, [esp + 4*4 + 8] ; pattern
        cld
        ; check module for all uri path
@@:
        cmpsb
        jne     .check_special_char

        cmp     byte[edi - 1], 0
        jne     @b
.end_scan:
        sub     ebp, [esp + 4*4 + 12]
        shr     ebp, 3 ; div 8
        mov     eax, ebp
        pop     edi esi ebp ebx
        ret     16

.found_last_substr:
        add     ebp, 2*4  ; ptr + length
        cmp     ebp, ebx
        ja     .end_scan

        dec     esi
        mov     [ebp - 8], esi

        mov     ecx, esi
@@:
        cmp     byte[esi], 0
        lea     esi, [esi + 1]
        jne     @b

        inc     ecx
        sub     esi, ecx
        mov     [ebp - 4], esi

        jmp     .end_scan

.check_special_char:
.found_special_char:
        cmp     byte[edi - 1], '*'
        jne     .no_special_char

        ; check next char in uri of modules
        cmp     byte[edi], 0
        je      .found_last_substr  ; case "/file.txt/i.png"  "/file.txt*"
        ; uri path "*name" or "*name*"
        dec     esi

        add     ebp, 2*4  ; ptr + length
        cmp     ebp, ebx
        ja      .special_char_loop

        mov     [ebp - 8], esi
        mov     dword[ebp - 4], 0  ; if length(*) substr = 0

.special_char_loop:
        cmp     byte[esi], 0
        je      .fail_uri ; case "/file"  "/file*.open"

        xor     ecx, ecx
@@:
        inc     ecx
        mov     dl, byte[edi + ecx - 1]

        cmp     dl, '*'
        je      .find_next_spec_char

        cmp     byte[esi + ecx - 1], dl
        lea     esi, [esi + 1]
        jne     .special_char_loop
        dec     esi

        test    dl, dl
        jnz     @b

        ; case "/file/text.txt"  "/file*.txt"
        cmp     ebp, ebx
        ja      .end_scan

        mov     ecx, esi
        sub     ecx, [ebp - 8]
        mov     [ebp - 4], ecx

        jmp     .end_scan

.find_next_spec_char:
        ; found substr
        ; case "/file1234567/text.txt"  "/file*/text*"
        cmp     ebp, ebx
        ja      @f

        mov     eax, esi
        sub     eax, [ebp - 8]
        mov     [ebp - 4], eax
@@:
        add     esi, ecx
        add     edi, ecx

        jmp     .found_special_char

.fail_uri:
.no_special_char:
        pop     edi esi ebp ebx

        mov     eax, -1
        ret     16






