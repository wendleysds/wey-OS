#include <stdio.h>

int fprintf(FILE *restrict stream, const char *restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int count = vfprintf(stream, fmt, args);
    va_end(args);

    return count;
}