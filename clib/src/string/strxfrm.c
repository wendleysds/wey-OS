#include <string.h>

size_t strxfrm(char* dest, const char* src, size_t n){
    size_t src_len = strlen(src);
    if (n == 0) {
        return src_len;
    }

    if (dest != NULL) {
        size_t copy_len = (src_len < n) ? src_len : (n - 1);
        
        memcpy(dest, src, copy_len);
        dest[copy_len] = '\0';
    }

    return src_len;
}