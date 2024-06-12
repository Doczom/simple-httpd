
BASE_ARRAY_ARGS         equ (esi - 1024)
BASE_ARRAY_HEADERS      equ (esi - 2048)

MAX_COUNT_ARG           = 1024/(4+4)
MAX_COUNT_HEADER        = 1024/(4+4)

;TODO: fix checking end http packed 

; IN:
;    esi - struct
;    ecx - ptr to str URI
; OUT:
;    ecx = new base for reading data ('HTTP/1.1 ...')
;    eax = -1 - error 
;           0 - good
parse_url:
;      URI         = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
;      hier-part   = "//" authority path-abempty
;                  / path-absolute
;                  / path-rootless
;                  / path-empty 
;         foo://example.com:8042/over/there?name=ferret#nose
;         \_/   \______________/\_________/ \_________/ \__/
;          |           |            |            |        |
;       scheme     authority       path        query   fragment
 
        cmp     byte[ecx], '/' ; check abs-path
        je      .get_path
        
        ;get scheme
        mov     [esi + CONNECT_DATA.uri_scheme], ecx
@@:
        cmp     ecx, [esi + CONNECT_DATA.end_buffer_request]
        ja      .error_exit

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
        cmp     ecx, [esi + CONNECT_DATA.end_buffer_request]
        ja      .error_exit

        inc     ecx
        ;cmp     byte[ecx - 1], ' ' ;check end, in http always is used abs-path

        cmp     byte[ecx - 1], '/'
        jne     @b
        dec     ecx
.get_path:
        ;ecx = path-absolute 
        mov     [esi + CONNECT_DATA.uri_path], ecx
        mov     edi, ecx
        mov     al, byte[ecx]
.get_path.new_char:
        stosb
        inc     ecx

        cmp     ecx, [esi + CONNECT_DATA.end_buffer_request]
        ja      .error_exit

        mov     al, byte[ecx]
        mov     byte[edi], 0

        cmp     al, '?'
        je      .get_query
        
        cmp     al, '#'
        je      .get_fragment

        cmp     al, ' ' ; check end path
        je      .get_path.end

        cmp     al, '%' ; check end path
        jne     .get_path.new_char

        ; %00-%FF
        inc     ecx
        movzx   eax, byte[ecx]
        mov     al, byte[hex_table + eax]
        cmp     al, -1
        je      .error_exit
        mov     edx, eax

        inc     ecx
        movzx   eax, byte[ecx]
        mov     al, byte[hex_table + eax]
        cmp     al, -1
        je      .error_exit
        shl     edx, 4
        add     eax, edx

        jmp     .get_path.new_char
.get_path.end:
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

        cmp     edx, MAX_COUNT_HEADER 
        jae     .error_exit

        mov     dword[BASE_ARRAY_ARGS + (edx-1)*8], ecx
        mov     [esi + CONNECT_DATA.num_uri_args], edx
        dec     ecx
@@:
        inc     ecx

        cmp     ecx, [esi + CONNECT_DATA.end_buffer_request]
        ja      .error_exit

        cmp     byte[ecx], '='
        je      .get_args

        cmp     byte[ecx], '#'  ; THIS IS COMPLETE NONSENSE, BUT SUDDENLY
        je      .get_fragment

        cmp     byte[ecx], ' '  ; http://cjkhr.bvgbfdkvdf.dmejfgehf/?1pr1
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

        cmp     ecx, [esi + CONNECT_DATA.end_buffer_request]
        ja      .error_exit

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
        cmp     ecx, [esi + CONNECT_DATA.end_buffer_request]
        ja      .error_exit
        
        inc     ecx
        cmp     byte[ecx - 1], ' '
        jne     @b
        mov     byte[ecx - 1], 0
 .exit:
        xor     eax, eax 
        ret

.error_exit:
        ; set return value
        mov     eax, -1
        ret

; IN:
;    esi - struct
;    ecx - ptr to begin headers block
; OUT:
;    ecx = new base for reading body message HTTP query
;    eax = -1 - error
;           0 - good
parse_headers:
        ; init array
        mov     [esi + CONNECT_DATA.num_headers], 0
        lea     eax, [BASE_ARRAY_HEADERS]
        mov     [esi + CONNECT_DATA.http_headers], eax
        xor     edx, edx

        ; for check size
        mov     eax, [esi + CONNECT_DATA.end_buffer_request]
 .new_str:       
        cmp     ecx, eax
        jae     .error_exit
        
        cmp     word[ecx], 0x0A0D ; \n
        jnz     .find_header
 .exit:
        ; end find heeaders
        mov     byte[ecx], 0 
        add     ecx, 2 ; ecx = base for body message
        xor     eax, eax
        ret
.error_exit:
        mov     eax, -1
        ret     

.find_header:
        ; add new item in array headers
        cmp     edx, MAX_COUNT_HEADER 
        jae     .error_exit

        inc     edx
        mov     dword[esi + CONNECT_DATA.num_headers], edx
        ; save pointer to name header
        mov     dword[BASE_ARRAY_HEADERS + (edx-1)*8], ecx
        dec     ecx
 @@:       
        inc     ecx
        ; check size
        cmp     ecx, eax
        jae     .error_exit

        cmp     byte[ecx], ':'
        jnz     @b

        mov     byte[ecx], 0 ; \0
@@:
        inc     ecx

        cmp     ecx, eax
        jae     .error_exit

        cmp     byte[ecx], ' '
        je      @b

        ; save pointer to value
        mov     dword[BASE_ARRAY_HEADERS + (edx-1)*8 + 4], ecx
@@:
        ; check size
        cmp     ecx, eax
        jae     .error_exit
        
        inc     ecx
        cmp     word[ecx - 1], 0x0A0D
        jnz     @b

        cmp     byte[ecx + 1], ' '  ; SP
        je      @b
        cmp     byte[ecx + 1], 0x09 ; HT
        je      @b

        mov     byte[ecx - 1], 0
        inc     ecx ; set offset on new string
        jmp     .new_str

; IN: 
;    ecx - raw data query
;    esi - ptr to CONNECT_DATA 
; OUT: eax = 0 error
;      eax = prt to struct CONNECT_DATA
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
        cmp     ecx, [esi + CONNECT_DATA.end_buffer_request]
        ja      .error_exit

        ; ecx <- uri string
        ; parsing the URI string in the start line of the query 
        ; (getting the schema, path, arguments, fragment, etc.)
        call    parse_url
        test    eax, eax
        jnz     .error_exit

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
        test    eax, eax
        jnz     .error_exit

        ; check size
        cmp     ecx, [esi + CONNECT_DATA.end_buffer_request]
        je      @f

        mov     [esi + CONNECT_DATA.message_body], ecx
        mov     eax, [esi + CONNECT_DATA.end_buffer_request]
        sub     eax, ecx
        mov     [esi + CONNECT_DATA.message_body_len], eax
@@:        
        mov     eax, esi 
        ret

.error_exit:
        xor     eax, eax
        ret