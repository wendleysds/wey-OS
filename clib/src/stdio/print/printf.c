#include <stdio.h>

int printf(const char *restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int count = vfprintf(stdout, fmt, args);
    va_end(args);

    return count;
}
