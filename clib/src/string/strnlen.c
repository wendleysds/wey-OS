#include <string.h>

size_t strnlen(const char* s, size_t max) {
    size_t len = 0;
    while(len < max && *s) {
        len++;
        s++;
    }
    return len;
}
