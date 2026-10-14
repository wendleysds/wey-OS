#include <stdio.h>

char *fgets(char *restrict str, int size, FILE *restrict stream) {
    if (!str || size <= 0 || !stream) {
        return NULL;
    }

    if (size == 1) {
        str[0] = '\0';
        return str;
    }

    int i = 0;
    while (i < size - 1) {
        int c = fgetc(stream);

        if (c == EOF) {
            if (i == 0 || ferror(stream)) {
                return NULL;
            }
            break;
        }

        str[i++] = (char)c;

        if (c == '\n') {
            break;
        }
    }

    str[i] = '\0';
    return str;
}
