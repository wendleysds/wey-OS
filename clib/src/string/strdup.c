#include <string.h>
#include <stdlib.h>

char* strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *new = malloc(len);
    if (new == NULL) return NULL;
    return memcpy(new, s, len);
}