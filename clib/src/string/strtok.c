#include <string.h>

char *strtok_r(char *restrict s, const char *restrict delim, char **restrict saveptr) {
    char *end;

    if (s == NULL) {
        s = *saveptr;
    }

    if (s == NULL) {
        return NULL;
    }

    s += strspn(s, delim);
    if (*s == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    end = s + strcspn(s, delim);
    if (*end == '\0') {
        *saveptr = NULL;
        return s;
    }

    *end = '\0';
    *saveptr = end + 1;
    return s;
}

char *strtok(char *restrict s, const char *restrict delimiters) {
    static char *last = NULL;
    return strtok_r(s, delimiters, &last);
}