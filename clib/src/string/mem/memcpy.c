#include <string.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;

    while(n--){
        *d++ = *s++;
    }

    return dest;
}