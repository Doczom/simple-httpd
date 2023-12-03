

struct CONNECT_DATA ; 16*4 = 64 bytes
        socket          dd 0 ; номер сокета подключения
        sockaddr        dd 16/4 ; socaddr connection
        buffer_request  dd 0 ; pointer to buffer for geting message socket
        request_size    dd 0 ; size geted data from client
        end_buffer_request dd 0 ; для парсера 
        buffer_response dd 0 ; pointer to buffwr for resp message 
        http_method     dd 0 ; указатель на строку
        http_verion     dd 0 ; указатель на строку
        num_headers     dd 0 ; number items in REQUEST_DATA
        http_headers    dd 0 ; указатель на массив REQUEST_DATA
        uri_scheme      dd 0 ; указатель на схему
        uri_authority   dd 0 ; pointer to struct ?
        uri_path        dd 0 ; указатель на декодированный путь к ресурсу(без параметров)
        num_uri_args    dd 0 ;
        uri_arg         dd 0 ; pointer to array REQUEST_DATA аргументов uri строк
        uri_fragment    dd 0 ; указатель на строку
        message_body    dd 0 ; указатель на тело http запроса
ends

struct HTTPD_UNIT
        next            rd 1
        prev            rd 1
        httpd_serv      rd 1
        uri_path        rb 4096-3*4
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

        sub     esp, 16
        mov     esi, esp
        invoke  ini.get_str, ebp, ini_section_main, ini_key_ip, esi, 16, 0 ; ip
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
        
        invoke  ini.get_int, ebp, ini_section_main, ini_key_port, 80 ; standart port
        xchg    al, ah
        mov     [srv_sockaddr.port], ax
        
        invoke  ini.get_int, ebp, ini_section_main, ini_key_conn, 10 ; standart port
        mov     [srv_backlog], ax

        ; flags

        ; work_dir
        invoke  ini.get_str, ebp, ini_section_main, ini_key_work_dir, GLOBAL_DATA.work_dir, 1024, 0 
        push    edi
        mov     ecx, 1024
        mov     edi, GLOBAL_DATA.work_dir
        xor     eax, eax
        repne scasb
        dec     edi
        sub     edi, GLOBAL_DATA.work_dir
        mov     [GLOBAL_DATA.work_dir.size], edi
        pop     edi

        ; TODO: get mime file
        ;mov     dword[GLOBAL_DATA.MIME_types_arr], STD_MIME_TYPE_ARR
        call    load_mime_file

        ; units_dir
        invoke  ini.get_str, ebp, ini_section_main, ini_key_units_dir, GLOBAL_DATA.unit_dir, 1024, 0 
        push    edi
        mov     ecx, 1024
        mov     edi, GLOBAL_DATA.unit_dir
        xor     eax, eax
        repne scasb
        mov     byte[edi-1], '/'
        mov     [GLOBAL_DATA.unit_dir.end], edi
        pop     edi

        ; get all units
        invoke  ini.enum_keys, ebp, ini_section_units, .add_unit

        xor     eax, eax
        ret

.add_unit: ; [esp + 4*3] - name [esp + 4*4] - value
        ; add new item in list 
        push    dword sizeof.HTTPD_UNIT
        call    Alloc
        test    eax, eax
        jz      .add_unit.exit 
        
        mov     ecx, [GLOBAL_DATA.units]
        mov     [eax + HTTPD_UNIT.next], ecx
        mov     dword[eax + HTTPD_UNIT.prev], GLOBAL_DATA.units
        mov     [GLOBAL_DATA.units], eax
        test    ecx, ecx
        jnz     @f
        mov     [ecx + HTTPD_UNIT.prev], eax
@@:
        ; copy uri path
        push    esi edi
        mov     esi, [esp + 4*2 + 4*3] ; name
        lea     edi, [eax + HTTPD_UNIT.uri_path + 1]
        mov     byte[edi - 1], '/'
@@:
        movsb
        cmp     byte[edi - 1], 0
        jne     @b
        ; copy file name
        mov     edi, [GLOBAL_DATA.unit_dir.end]
        mov     esi, [esp + 4*2 + 4*4]
@@:
        movsb
        cmp     byte[edi - 1], 0
        jne     @b

        pop     edi esi
        mov     esi, eax 
        ; load library
        push    esi
        stdcall dll.Load, IMPORT_UNIT
        pop     esi
        test    eax, eax
        jz      @f

 .add_unit.err:
        ; error
        mov     eax, [esi + HTTPD_UNIT.next] ; next
        mov     [GLOBAL_DATA.units], eax 
        mov     dword[eax + HTTPD_UNIT.prev], GLOBAL_DATA.units
        
        push    esi
        call    Free
        
        jmp     .add_unit.exit 
@@: ; good
        ; init httpd unit
        push    dword EXPORT_DATA
        invoke  httpd_import.init
        test    eax, eax 
        jnz      .add_unit.err

        mov     eax, [httpd_import.serv]
        mov     dword[esi + HTTPD_UNIT.httpd_serv], eax

        mov     [httpd_import.init], httpd_unit_init
        mov     [httpd_import.serv], httpd_unit_serv
.add_unit.exit:        
        ret 16



.err:
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
        lea     ecx, [eax + sizeof.FILED + 40]
        mov     [esi + FILED.path], ecx
        invoke  ini.get_str, ebp, ini_section_main, ini_key_mime_file, ecx, 1024, 0 
        test    eax, eax
        jnz     .no_file

        lea     ecx, [esi + sizeof.FILED]
        mov     [esi + FILED.buffer], ecx
        stdcall FileInfo, esi
        test    eax, eax
        jnz     .no_file

        mov     ecx, [esi + sizeof.FILED + 32]
        stdcall Alloc, ecx
        test    eax, eax 
        jz      .no_file

        mov     ecx, [esi + sizeof.FILED + 32]
        mov     edi, eax
        mov     [esi + FILED.buffer], eax
        mov     [esi + FILED.size], ecx
        mov     [esi + FILED.offset], 0
        mov     dword[esi + FILED.offset + 4], 0
        stdcall FileRead, esi
        test    eax, eax
        jnz     .error_read

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
        stdcall Free, esi
.err:
        mov     dword[GLOBAL_DATA.MIME_types_arr], STD_MIME_TYPE_ARR
        ret