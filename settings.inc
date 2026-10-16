

struct HTTPD_HOST
        list_module     rd 1   ; singly linked list

        host            rb 256 ; 255 max length of domain name
ends

struct HTTPD_TRANSPORT
        next            rd 1
        pdata           rd 1
        httpd_t_close   rd 1
ends


struct HTTPD_MODULE
        next            rd 1
        prev            rd 1
        httpd_close     rd 1
        httpd_serv      rd 1
        pdata           rd 1
        uri_path        rb 4096-5*4
ends
diff10 'sizeof.HTTPD_MODULE: ', 0, sizeof.HTTPD_MODULE

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
diff10 'sizeof.RESPD: ', 0, sizeof.RESPD


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
        
        invoke  ini.enum_keys, ebp, ini_section_tls, .ini_enum_keys_callback_exit
        test    eax, eax
        jnz     @f
        mov     dword[srv_tls_enable], 1
@@:
        mov     eax, 80
        cmp     dword[srv_tls_enable], 0
        je      @f
        mov     eax, 443
@@:
        invoke  ini.get_int, ebp, ini_section_main, ini_key_port, eax ; default port
        xchg    al, ah
        mov     [srv_sockaddr.port], ax
        
        invoke  ini.get_int, ebp, ini_section_main, ini_key_conn, 10 ; default value
        mov     [srv_backlog], ax

        ; flags

        ; work_dir
        call    load_work_dir
        DEBUGF  1, "HTTPD: work dir: %s\n", GLOBAL_DATA.work_dir

        ; get mime file
        call    load_default_mime_file

        ; units_dir
        call    load_modules_dir
        DEBUGF  1, "HTTPD: modules dir: %s\n", GLOBAL_DATA.modules_dir

        ; get all units
        invoke  ini.enum_keys, ebp, ini_section_units, settings_add_module
.no_modules:
        xor     eax, eax
        ret
.error_exit2:
        add     esp, 16
.error_exit:
        mov     eax, -1
        ret
.ini_enum_keys_callback_exit:
        ret 16

settings_add_host:  ; [esp + 4*2] - name section


settings_add_module: ; [esp + 4*3] - name [esp + 4*4] - value
        ; skip configuration directives
        mov     eax, [esp + 4*3]
        cmp     byte[eax], '@'
        je      .exit

        ; add new item in list
        push    dword sizeof.HTTPD_MODULE
        call    Alloc
        test    eax, eax
        jz      .exit
        
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
        lea     edi, [eax + HTTPD_MODULE.uri_path]
@@:
        movsb
        cmp     byte[edi - 1], 0
        jne     @b

        ; copy file name
        mov     esi, [esp + 4*2 + 4*4]

        sub     esp, 1024*6 ; 6kib for "/../", "/./" etc
        mov     edi, esp

        cmp     byte[esi], '/'
        jz      .copy_value

        cmp     byte[esi], '@'
        jz      .copy_value

        push    esi
        mov     esi, GLOBAL_DATA.modules_dir
@@:
        movsb
        cmp     byte[edi - 1], 0
        jne     @b

        dec     edi
        pop     esi

        ; copy value in buffer
        ; edi - buffer esi - substr eax - this
.copy_value:
        xor     edx, edx
@@:
        movsb
        cmp     byte[esi - 1], 0
        je     @f

        cmp     byte[esi - 1], ','
        jne     @b

        mov     byte[edi - 1], 0
        mov     edx, esi
@@:
        mov     esi, eax
        ; edx - cmdline esp - path esi-this
        mov     ecx, esp
        push    edx

        cmp     byte[ecx], '@'
        jz      .detect_embedded_modules

        ; load library
        stdcall LoadLibrary, ecx
        test    eax, eax
        jz      .err_load
        mov     edi, eax

        stdcall dll.GetProcAddress, edi, httpd_module_serv
        test    eax, eax
        jz      .err_format
        mov     dword[esi + HTTPD_MODULE.httpd_serv], eax

        stdcall dll.GetProcAddress, edi, httpd_module_close
        test    eax, eax
        jz      .err_format
        mov     dword[esi + HTTPD_MODULE.httpd_close], eax

        stdcall dll.GetProcAddress, edi, httpd_module_init
        test    eax, eax
        jz      .err_format

.init_module: ; [esp] = cmdline esp+4=path esi-this
        ; init httpd module
        mov     edx, [esp] ; cmdline

        lea     ecx, [esp + 4]
        push    ecx ; path to dll
        lea     ecx, [esi + HTTPD_MODULE.uri_path]
        push    dword 0 ; host
        push    ecx
        push    edx
        push    dword EXPORT_DATA
        stdcall eax

        test    eax, eax 
        jz      .err_init

        mov     [esi + HTTPD_MODULE.pdata], eax

        add     esp, 6*1024 + 4
        pop     edi esi

.exit:
        ret 16

.detect_embedded_modules: ;esi - this ecx - path [esp] - cmdline
        ; find emmbedded module
        mov     edi, EMBEDDED_MODULES
        xor     edx, edx
        sub     edi, 16
.detect_embedded_modules.next:
        add     edi, 16
        mov     eax, [edi]
        test    eax, eax
        jz      .err_load ; not found

        lea     ecx, [esp + 4]
@@:
        mov     dl, [eax]
        cmp     byte[ecx], dl
        jnz     .detect_embedded_modules.next

        inc     ecx
        inc     eax
        test    edx, edx
        jnz     @b

        mov     eax, [edi + 4]
        jmp     .init_module

.err_format:
        lea     ecx, [esp + 4]
        DEBUGF  1, "HTTPD: Load modules: bad format path=%s cmd=%s\n", ecx, [esp]
        jmp     .err
.err_load:
        lea     ecx, [esp + 4]
        DEBUGF  1, "HTTPD: Load modules: error loading dll path=%s cmd=%s\n", ecx, [esp]
        jmp     .err
.err_init:
        lea     ecx, [esp + 4]
        DEBUGF  1, "HTTPD: Load modules: httpd_init return error code path=%s cmd=%s\n", ecx, [esp]
.err:
        add     esp, 6*1024 + 4

        ; error. Removing this structure from the list
        mov     eax, [esi + HTTPD_MODULE.next] ; next
        mov     [GLOBAL_DATA.modules], eax
        mov     dword[eax + HTTPD_MODULE.prev], GLOBAL_DATA.modules
        
        push    esi
        call    Free
        
        ; Проверка "жёсткости" установок: игнор или выкидываем ошибку

        pop     edi esi
        jmp     .exit

; in: ebp - path to config file
load_work_dir:
        ; work_dir
        invoke  ini.get_str, ebp, ini_section_main, ini_key_work_dir, GLOBAL_DATA.work_dir, 4096, 0
        test    eax, eax
        jnz     .not_found

        mov     [GLOBAL_DATA.work_dir.size], 0
        cmp     byte[GLOBAL_DATA.work_dir], '/'
        je      .find_size

        call    .get_curr_dir

        mov     byte[GLOBAL_DATA.work_dir + eax], '/'
        inc     eax
        lea     edx, [GLOBAL_DATA.work_dir + eax]
        mov     ecx, 4096
        sub     ecx, eax

        invoke  ini.get_str, ebp, ini_section_main, ini_key_work_dir, edx, ecx, 0

.find_size:
        push    edi
        mov     ecx, 4096
        mov     edi, GLOBAL_DATA.work_dir
        xor     eax, eax
        repne scasb
        dec     edi
        sub     edi, GLOBAL_DATA.work_dir
        mov     [GLOBAL_DATA.work_dir.size], edi
        pop     edi
        ret
.get_curr_dir:
.not_found:
        stdcall GetCurrentDir, GLOBAL_DATA.work_dir, 4096
        mov     [GLOBAL_DATA.work_dir.size], eax
        ret

; IN: ebp - path to config file
load_modules_dir:
        invoke  ini.get_str, ebp, ini_section_main, ini_key_modules_dir, GLOBAL_DATA.modules_dir, 4096-1, 0
        test    eax, eax
        jnz     .load_image_dir

        cmp     byte[GLOBAL_DATA.modules_dir], '/'
        je      .exit

        stdcall load_image_dir, GLOBAL_DATA.modules_dir

        mov     edx, 4096-1
        sub     edx, ecx
        lea     eax, [GLOBAL_DATA.modules_dir + ecx]

        invoke  ini.get_str, ebp, ini_section_main, ini_key_modules_dir, eax, edx, 0

.exit:
        push    esi edi
        mov     ecx, 4096
        mov     edi, GLOBAL_DATA.modules_dir
        xor     eax, eax
        repne scasb

        cmp     byte[edi - 2], '/'
        jz      @f

        mov     byte[edi-1], '/'
@@:
        pop     edi esi
        ret

;  IN: [esp + 4] - pointer to buffer(>1024 bytes)
; OUT: ecx = length image path
load_image_dir:
        push    esi edi
        mov     edx, 1024
        mov     ecx, 1024
        mov     edi, IMAGE_PATH
        xor     eax, eax
        repnz scasb
        mov     esi, IMAGE_PATH
        mov     edi, [esp + 4*2 + 4]
        sub     edx, ecx
        mov     ecx, edx
        rep movsb
        ; find finish '/'
        mov     ecx, edx
        mov     eax, '/'
        std
        repnz scasb
        cld
        add     ecx, 2
        mov     byte[edi + 2], 0
        pop     edi esi
        ret     4

; IN: ebp - path to config file
load_default_mime_file:
        push    esi
        sub     esp, 4096
        mov     esi, esp

        invoke  ini.get_str, ebp, ini_section_main, ini_key_mime_file, esi, 4096, 0
        test    eax, eax
        jnz     .not_found

        cmp     byte[esi], '/'
        jz      .load_file

        ;copy work_dir
        push    esi
        mov     edi, esi
        mov     esi, GLOBAL_DATA.work_dir
@@:
        movsb
        cmp     byte[edi - 1], 0
        jne     @b

        mov     byte[edi - 1], '/'
        pop     esi

        lea     eax, [esi + 4096]
        sub     eax, edi

        invoke  ini.get_str, ebp, ini_section_main, ini_key_mime_file, edi, eax, 0
        test    eax, eax
        jnz     .not_found

.load_file:
        DEBUGF  1, 'HTTPD: mime_file=%s\n', esi
        stdcall load_mime_file, esi
        test    eax, eax
        jz      .not_found

        add     esp, 4096
        pop     esi
        mov     dword[GLOBAL_DATA.MIME_types_arr], eax
        ret

.not_found:
.err:
        add     esp, 4096
        pop     esi
        mov     dword[GLOBAL_DATA.MIME_types_arr], STD_MIME_TYPE_ARR
        ret

; void* load_mime_file(char* file);
load_mime_file:
        push    esi edi
        mov     esi, [esp + 4*2 + 4]
        sub     esp, 40

        stdcall FileInfo, esi, esp
        test    eax, eax
        jnz     .no_file

        mov     ecx, dword[esp + FILEINFO.Size]
        stdcall Alloc, ecx
        test    eax, eax 
        jz      .no_file

        mov     ecx, dword[esp + FILEINFO.Size]
        mov     edi, eax

        stdcall FileReadOfName, esi, edi, ecx
        test    eax, eax
        jz      .error_read

; edi - base in memory
.reloc_array:
        mov     edx, dword[esp + FILEINFO.Size]
        mov     esi, edi
@@:
        cmp     [edi], edx
        jae     .error_load

        add     [edi], esi
        mov     ecx, [edi]
        add     edi, 4
        cmp     dword[ecx], 0
        jne     @b

        add     esp, 40
        mov     eax, esi
        pop     edi esi
        ret     4

.error_read:
.error_load:
        stdcall Free, edi
.no_file:
.err_exit:
        xor     eax, eax
        add     esp, 40
        pop     edi esi
        ret     4