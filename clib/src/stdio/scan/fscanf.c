#include <stdio.h>

int fscanf(FILE *restrict stream, const char *restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vfscanf(stream, fmt, args);
    va_end(args);
    return ret;
}