#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

ssize_t getdelim(char **restrict lineptr, size_t *restrict n, int delimiter, FILE *restrict stream) {
    if (lineptr == NULL || n == NULL || stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (*lineptr == NULL || *n == 0) {
        size_t init_len = 128;
        char *new_ptr = realloc(*lineptr, init_len);
        if (new_ptr == NULL) {
            return -1;
        }
        *lineptr = new_ptr;
        *n = init_len;
    }

    size_t pos = 0;
    int c;

    while ((c = fgetc(stream)) != EOF) {
        if (pos + 1 >= *n) {
            if (*n > SIZE_MAX / 2) {
                errno = EOVERFLOW;
                return -1;
            }
            size_t new_size = *n * 2;
            char *new_ptr = realloc(*lineptr, new_size);
            if (new_ptr == NULL) {
                return -1;
            }
            *lineptr = new_ptr;
            *n = new_size;
        }

        (*lineptr)[pos++] = (char)c;

        if (c == delimiter) {
            break;
        }
    }

    if (pos == 0 || ferror(stream)) {
        return -1;
    }

    if (pos > LONG_MAX) {
        errno = EOVERFLOW;
        return -1;
    }

    (*lineptr)[pos] = '\0';
    return (ssize_t)pos;
}
