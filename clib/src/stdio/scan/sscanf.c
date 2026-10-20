#include <stdio.h>

int sscanf(const char *restrict s, const char *restrict format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsscanf(s, format, args);
    va_end(args);
    return ret;
}