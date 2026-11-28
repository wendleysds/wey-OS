#include <stdlib.h>
#include <string.h>

char* strndup(const char *s, size_t n) {
    size_t len = strlen(s) + 1;
    if (len > n) {
        len = n;
    }

    char* new_str = (char*) malloc(len);
    if (new_str == NULL) {
        return NULL;
    }

    memcpy(new_str, s, len);
    new_str[len] = '\0';

    return new_str;
}