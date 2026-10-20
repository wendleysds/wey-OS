#include <stdio.h>

int snprintf(char *restrict buf, size_t size, const char *restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int count = vsnprintf(buf, size, fmt, args);
    va_end(args);

    return count;
}