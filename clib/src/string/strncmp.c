#include <string.h>

int strncmp(const char* a, const char* b, size_t n){
    while(n-- && *a && *b){
        if(*a != *b){
            return *a - *b;
        }
        a++;
        b++;
    }

    return *a - *b;
}