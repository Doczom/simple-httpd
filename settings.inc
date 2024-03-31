



struct HTTPD_MODULE
        next            rd 1
        prev            rd 1
        httpd_close     rd 1
        httpd_serv      rd 1
        pdata           rd 1
        uri_path        rb 4096-5*4
ends

struct RESPD
        session         rd 1
        flags           rd 1
        http_status     rd 1
        http_ver_ptr    rd 1
        http_ver_len    rd 1
        buffer          rd 1
        buffer_size     rd 1
        http_body       rd 1
        count_header    rd 1
        header.ptr      rd 1
        header.len      rd 1
                        rd 2*(64 - 1)
ends


struct REQUEST_DATA
        ptr_name        dd 0 ;
        ptr_data        dd 0 ;
ends

; Load server config 
; ecx - path to file
; OUT: eax - 0 or err_code
load_settings:
        mov     ebp, ecx

        mov     dword[srv_stop], 0
        mov     dword[srv_shutdown], 0
        mov     dword[srv_tls_enable], 0

        sub     esp, 16
        mov     esi, esp
        invoke  ini.get_str, ebp, ini_section_main, ini_key_ip, esi, 16, 0 ; ip
        test    eax, eax
        jnz     .error_exit2
        ; xxx.xxx.xxx.xxx\n - 16 byte max
        xor     edx, edx
        xor     eax, eax
        mov     ecx, 4 ; count '.'
@@:     
        add     al, [esp]
        sub     al, '0'
        inc     esp
        mul     dword[_DIV_10_]

        cmp     byte[esp], '0' ; if . , space and other 
        jae     @b

        mov     byte[srv_sockaddr.ip], al
        ror     dword[srv_sockaddr.ip], 8
        add     esp, 1
        dec     ecx
        jnz     @b

        mov     esp, esi
        add     esp, 16


        mov     word[srv_sockaddr], AF_INET4
        
        invoke  ini.enum_keys, ebp, ini_section_tls, .add_module.exit
        test    eax, eax
        jnz     @f
        mov     dword[srv_tls_enable], 1
@@:
        mov     eax, 80
        cmp     dword[srv_tls_enable], 0
        je      @f
        mov     eax, 443
@@:
        invoke  ini.get_int, ebp, ini_section_main, ini_key_port, eax ; standart port
        xchg    al, ah
        mov     [srv_sockaddr.port], ax
        
        invoke  ini.get_int, ebp, ini_section_main, ini_key_conn, 10 ; standart port
        mov     [srv_backlog], ax

        ; flags

        ; work_dir
        invoke  ini.get_str, ebp, ini_section_main, ini_key_work_dir, GLOBAL_DATA.work_dir, 1024, 0
        test    eax, eax
        jnz     .error_exit

        push    edi
        mov     ecx, 1024
        mov     edi, GLOBAL_DATA.work_dir
        xor     eax, eax
        repne scasb
        dec     edi
        sub     edi, GLOBAL_DATA.work_dir
        mov     [GLOBAL_DATA.work_dir.size], edi
        pop     edi

        ; get mime file
        call    load_mime_file

        ; units_dir
        invoke  ini.get_str, ebp, ini_section_main, ini_key_modules_dir, GLOBAL_DATA.modules_dir, 1024, 0
        test    eax, eax
        jnz     .no_modules

        push    edi
        mov     ecx, 1024
        mov     edi, GLOBAL_DATA.modules_dir
        xor     eax, eax
        repne scasb
        mov     byte[edi-1], '/'
        mov     [GLOBAL_DATA.modules_dir.end], edi
        pop     edi

        ; get all units
        invoke  ini.enum_keys, ebp, ini_section_units, .add_module
.no_modules:
        xor     eax, eax
        ret

.add_module: ; [esp + 4*3] - name [esp + 4*4] - value
        ; add new item in list 
        push    dword sizeof.HTTPD_MODULE
        call    Alloc
        test    eax, eax
        jz      .add_module.exit
        
        mov     ecx, [GLOBAL_DATA.modules]
        mov     [eax + HTTPD_MODULE.next], ecx
        mov     dword[eax + HTTPD_MODULE.prev], GLOBAL_DATA.modules
        mov     [GLOBAL_DATA.modules], eax
        test    ecx, ecx
        jnz     @f
        mov     [ecx + HTTPD_MODULE.prev], eax
@@:
        ; copy uri path
        push    esi edi
        mov     esi, [esp + 4*2 + 4*3] ; name
        lea     edi, [eax + HTTPD_MODULE.uri_path + 1]
        mov     byte[edi - 1], '/'
@@:
        movsb
        cmp     byte[edi - 1], 0
        jne     @b
        ; copy file name
        mov     [GLOBAL_DATA._module_cmd], 0
        mov     edi, [GLOBAL_DATA.modules_dir.end]
        mov     esi, [esp + 4*2 + 4*4]
@@:
        movsb
        cmp     byte[edi - 1], 0
        je     @f

        cmp     byte[esi - 1], ';'
        jne     @b
        mov     byte[edi - 1], 0
        mov     [GLOBAL_DATA._module_cmd], esi
@@:
        pop     edi esi
        mov     esi, eax 
        ; load library
        push    esi
        stdcall dll.Load, IMPORT_MODULE
        pop     esi
        test    eax, eax
        jz      @f

 .add_module.err:
        ; error
        mov     eax, [esi + HTTPD_MODULE.next] ; next
        mov     [GLOBAL_DATA.modules], eax
        mov     dword[eax + HTTPD_MODULE.prev], GLOBAL_DATA.modules
        
        push    esi
        call    Free
        
        jmp     .add_module.exit
@@: ; good
        ; init httpd module
        push    dword[GLOBAL_DATA._module_cmd]
        push    dword EXPORT_DATA
        invoke  httpd_import.init

        test    eax, eax 
        jz      .add_module.err

        mov     [esi + HTTPD_MODULE.pdata], eax
        mov     eax, [httpd_import.serv]
        mov     dword[esi + HTTPD_MODULE.httpd_serv], eax
        mov     eax, [httpd_import.close]
        mov     dword[esi + HTTPD_MODULE.httpd_close], eax

        mov     [httpd_import.init], httpd_module_init
        mov     [httpd_import.serv], httpd_module_serv
        mov     [httpd_import.close], httpd_module_close
.add_module.exit:
        ret 16



.error_exit2:
        add     esp, 16
.error_exit:
        mov     eax, -1
        ret


load_mime_file:
        stdcall Alloc, 4096
        test    eax, eax
        jz      .err
        push    eax
        mov     edi, eax
        xor     eax, eax
        mov     ecx, 1024
        rep stosd 
        pop     eax

        mov     esi, eax
        invoke  ini.get_str, ebp, ini_section_main, ini_key_mime_file, esi, 1024, 0
        test    eax, eax
        jnz     .no_file

        sub     esp, 40
        stdcall FileInfo, esi, esp
        test    eax, eax
        jnz     .no_file

        mov     ecx, [esp + 32]
        stdcall Alloc, ecx
        test    eax, eax 
        jz      .no_file

        mov     ecx, [esp + 32]
        mov     edi, eax

        stdcall FileReadOfName, esi, edi, ecx
        test    eax, eax
        jz      .error_read

        add     esp, 40

        mov     dword[GLOBAL_DATA.MIME_types_arr], edi
        mov     eax, edi
@@:
        add     [edi], eax
        mov     ecx, [edi]
        add     edi, 4
        cmp     dword[ecx], 0
        jne     @b

        stdcall Free, esi
        ret
.error_read:
        stdcall Free, edi
.no_file:
        add     esp, 40
        stdcall Free, esi
.err:
        mov     dword[GLOBAL_DATA.MIME_types_arr], STD_MIME_TYPE_ARR
        ret