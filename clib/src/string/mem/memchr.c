#include <string.h>

char* memchr(const void* ptr, int c, size_t n){
    const unsigned char* p = (const unsigned char*)ptr;

    while(n--){
        if(*p == c){
            return (char*)p;
        }
        p++;
    }

    return NULL;
}