#include <sys/ksys.h>
#include "module_api.h"

static IMPORT_DATA funcs;

void* __memcpy(void* _dest, const void* _src, size_t _n){
    int d0, d1, d2;
    __asm__ __volatile__(
        "rep ; movsl\n\t"
        "testb $2,%b4\n\t"
        "je 1f\n\t"
        "movsw\n"
        "1:\ttestb $1,%b4\n\t"
        "je 2f\n\t"
        "movsb\n"
        "2:"
        : "=&c"(d0), "=&D"(d1), "=&S"(d2)
        : "0"(_n / 4), "q"(_n), "1"((long)_dest), "2"((long)_src)
        : "memory");
    return (_dest);
};

uint32_t __stdcall httpd_init(IMPORT_DATA* import, char* cmdline, char* uri_path, char* host, char* module_path){
	__memcpy(&funcs, import, import -> _sizeof);
	return 1;
};

char* text = "<p>Hello World</p><br><p>This module created on C lang</p>\0";

void __stdcall httpd_serv(void* request_data, uint32_t pdata){
	RESPD* ptr = funcs.create_resp(request_data, 0);
	funcs.send_resp(ptr, text, 58);
	funcs.destruct_resp(ptr);
	return;
};

void __stdcall httpd_close(uint32_t pdata){
	return;
};

ksys_dll_t EXPORTS[] = {
	{"httpd_init", &httpd_init},
	{"httpd_serv", &httpd_serv},
	{"httpd_close", &httpd_close},
	NULL
};