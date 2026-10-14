#include <string.h>

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