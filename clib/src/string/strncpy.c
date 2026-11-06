#include <string.h>

char *strncpy(char *d, const char *s, size_t n){
    size_t i = 0;
    for (; i < n && s[i]; i++) d[i] = s[i];
    for (; i < n; i++) d[i] = '\0';
    return d;
}