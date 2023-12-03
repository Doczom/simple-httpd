
BASE_ARRAY_ARGS         equ (esi - 1024)
BASE_ARRAY_HEADERS      equ (esi - 2048)

;TODO: fix checking end http packed 

; IN:
;    esi - struct
;    ecx = ptr to str URI
; OUT:
;    ecx - new base for reading data ('HTTP/1.1 ...')
;    eax - 
; NOTE: this function don`t check buffer size 
parse_url:
;      URI         = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
;
;      hier-part   = "//" authority path-abempty
;                  / path-absolute
;                  / path-rootless
;                  / path-empty 
;
;         foo://example.com:8042/over/there?name=ferret#nose
;         \_/   \______________/\_________/ \_________/ \__/
;          |           |            |            |        |
;       scheme     authority       path        query   fragment
;          |   _____________________|__
;         / \ /                        \
;         urn:example:animal:ferret:nose
 
        cmp     byte[ecx], '/' ; check abs-path
        je      .get_path
        
        ;get scheme
        mov     [esi + CONNECT_DATA.uri_scheme], ecx
@@:
        inc     ecx 
        cmp     byte[ecx - 1], ':'
        jne     @b
        mov     byte[ecx - 1], 0

        cmp     word[ecx], '//'
        jne     .get_path

        add     ecx, 2
        mov     [esi + CONNECT_DATA.uri_authority], ecx
        ;get authority
@@:
        inc     ecx
        ;cmp     byte[ecx - 1], ' ' ;check end, не нужно, так как в http всегда / абс путь 

        cmp     byte[ecx - 1], '/'
        jne     @b
        dec     ecx
.get_path:
        ;path-absolute
        mov     [esi + CONNECT_DATA.uri_path], ecx
@@:        
        inc     ecx
        cmp     byte[ecx], '?'
        je      .get_query
        
        cmp     byte[ecx], '#'
        je      .get_fragment

        cmp     byte[ecx], ' ' ; check end path
        jne     @b
        mov     byte[ecx], 0
        inc     ecx
        jmp     .exit

.get_query:
        mov     byte[ecx], 0
        inc     ecx
        lea     eax, [BASE_ARRAY_ARGS]
        mov     dword[esi + CONNECT_DATA.uri_arg], eax
        xor     edx, edx ; counter items
        ; add new item
.get_query_new_arg:
        inc     edx
        mov     dword[BASE_ARRAY_ARGS + (edx-1)*8], ecx
        mov     [esi + CONNECT_DATA.num_uri_args], edx
        dec     ecx
@@:
        inc     ecx
        cmp     byte[ecx], '='
        je      .get_args

        cmp     byte[ecx], '#'  ; ЭТО БРЕД ПОЛНЫЙ, НО ВДРУГ
        je      .get_fragment

        cmp     byte[ecx], ' '  ;  http://cjkhr.bvgbfdkvdf.dmejfgehf/?1pr1 
        jne     @b
.exit_2:
        mov     byte[ecx], 0
        inc     ecx
        jmp     .exit     

.get_args:
        mov     byte[ecx], 0
        inc     ecx
        mov     dword[BASE_ARRAY_ARGS + (edx-1)*8 + 4], ecx
        dec     ecx
@@:
        inc     ecx
        cmp     byte[ecx], '#'
        je      .get_fragment
        
        cmp     byte[ecx], ' '  
        je      .exit_2

        cmp     byte[ecx], '&'
        jne     @b

        mov     byte[ecx], 0
        inc     ecx

        jmp     .get_query_new_arg

.get_fragment:
        cmp     byte[ecx], '#'
        jne     .exit

        mov     byte[ecx], 0
        inc     ecx
        mov     [esi + CONNECT_DATA.uri_fragment], ecx     
@@:
        inc     ecx
        cmp     byte[ecx - 1], ' '
        jne     @b
        mov     byte[ecx - 1], 0
 .exit:
        ret


; IN:
;    esi - struct
;    ecx - ptr to begin headers block
;    edx - free mem ptr 
; OUT:
;    ecx - new base for reading body message HTTP query
;    eax -  
; NOTE: this function don`t check buffer size 
parse_headers:
        ; init array
        mov     [esi + CONNECT_DATA.num_headers], 0
        lea     eax, [BASE_ARRAY_HEADERS]
        mov     [esi + CONNECT_DATA.http_headers], eax
        xor     edx, edx

        ; for check size
        mov     eax, [esi + CONNECT_DATA.request_size]
        add     eax, [esi + CONNECT_DATA.buffer_request]

 .new_str:       
        cmp     ecx, eax
        jae     .exit
        
        cmp     word[ecx], 0x0A0D ; \n
        jnz     .find_header
 .exit:
        ; end find heeaders
        mov     byte[ecx], 0 
        add     ecx, 2 ; ecx = base for body message
        ret

.find_header:
        ; add new item in array headers
        cmp     edx, 512 ; max count headers
        jae     .exit

        inc     edx
        mov     dword[esi + CONNECT_DATA.num_headers], edx
        ; save pointer to name header
        mov     dword[BASE_ARRAY_HEADERS + (edx-1)*8], ecx
        dec     ecx
 @@:       
        inc     ecx

        cmp     ecx, eax
        jae     .exit

        cmp     byte[ecx], ':'
        jnz     @b

        mov     byte[ecx], 0 ; \0
        inc     ecx 
        ; save pointer to value
        mov     dword[BASE_ARRAY_HEADERS + (edx-1)*8 + 4], ecx
@@:
        cmp     ecx, eax
        jae     .exit
        
        inc     ecx
        cmp     word[ecx - 1], 0x0A0D
        jnz     @b
        mov     byte[ecx - 1], 0
        inc     ecx ; set offset on new string
        jmp     .new_str

; IN: 
;    ecx - raw data query
;    esi - ptr to CONNECT_DATA 
; OUT: eax = 0 error
;     eax = prt to struct CONNECT_DATA
parse_http_query:
;method scheme://host:port/abs_path HTTP/1.1 0x0d 0x0a
;header_1:value 0x0d 0x0a
; ...
;header_N:value 0x0d 0x0a
;0xd 0xa
; message data

        mov     eax, [esi + CONNECT_DATA.request_size]
        add     eax, [esi + CONNECT_DATA.buffer_request]
        mov     [esi + CONNECT_DATA.end_buffer_request], eax

        ; check size 
        cmp     dword[esi + CONNECT_DATA.request_size], min_http_size
        jb      .error_exit

        ; get http METHOD this message
        mov     [esi + CONNECT_DATA.http_method], ecx
@@:     
        cmp     ecx, [esi + CONNECT_DATA.end_buffer_request]
        ja      .error_exit
        
        inc     ecx
        cmp     byte[ecx - 1], ' ' ; find end method
        jnz     @b
        mov     byte[ecx - 1], 0 
        
        ; check size
        mov     edx, ecx
        sub     edx, [esi + CONNECT_DATA.buffer_request]
        sub     edx, 2 ; / 0x20
        cmp     dword[esi + CONNECT_DATA.request_size], edx
        jle     .error_exit

        ; ecx <- uri string
        ; парсинг uri строки в заголовке запроса(получение схемы, пути аргументов, фрагмента и тд)
        call    parse_url

        ; get http version(HTTP/1.1)
        mov     [esi + CONNECT_DATA.http_verion], ecx
@@:     
        cmp     ecx, [esi + CONNECT_DATA.end_buffer_request]
        ja      .error_exit
        
        inc     ecx
        cmp     word[ecx - 1], 0x0A0D
        jnz     @b
        mov     word[ecx - 1], 0
        inc     ecx ; <- start first header

        ; check size
        mov     edx, ecx
        sub     edx, [esi + CONNECT_DATA.buffer_request]
        sub     edx, 2 ; 0x0d 0x0a
        cmp     dword[esi + CONNECT_DATA.request_size], edx
        jle     .error_exit

        ; get headers request (key + value string)
        call    parse_headers


        ; check size
        cmp     ecx, [esi + CONNECT_DATA.end_buffer_request]
        je      @f

        mov     [esi + CONNECT_DATA.message_body], ecx
@@:        
        mov     eax, esi 
        ret

.error_exit:
        xor     eax, eax
        ret