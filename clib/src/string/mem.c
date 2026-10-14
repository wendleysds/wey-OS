#include <string.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;

    while(n--){
        *d++ = *s++;
    }

    return dest;
}

int memcmp(const void *a, const void *b, size_t n) {
    const char* ca = (const char*)a;
    const char* cb = (const char*)b;

    while(n--){
        if(*ca++ != *cb++){
            return *ca - *cb;
        }
    }

    return 0;
}

void *memset(void *dest, int c, size_t n) {
    unsigned char* d = (unsigned char*)dest;

    while(n--){
        *d++ = c;
    }

    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;

    if(d < s){
        while(n--){
            *d++ = *s++;
        }
    }else{
        d += n;
        s += n;

        while(n--){
            *--d = *--s;
        }
    }

    return dest;
}