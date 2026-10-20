#include <stdio.h>

int scanf(const char *restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vscanf(fmt, args);
    va_end(args);
    return ret;
}