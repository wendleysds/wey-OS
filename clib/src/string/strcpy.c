#include <string.h>

char* strcpy(char* dest, const char* src){
    char* d = dest;

    while((*d++ = *src++));

    return dest;
}