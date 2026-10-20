#include <string.h>

char* strncpy(char* dest, const char* src, size_t n){
    char* d = dest;

    while(n-- && (*d++ = *src++));
    while(n--) *d++ = '\0';

    return dest;
}