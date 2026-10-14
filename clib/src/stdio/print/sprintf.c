#include <stdio.h>

int sprintf(char *restrict str, const char *restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int count = vsprintf(str, fmt, args);
    va_end(args);

    return count;
}