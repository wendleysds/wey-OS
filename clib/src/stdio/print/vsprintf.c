#include <stdio.h>

int vsprintf(char *restrict str, const char *restrict fmt, va_list args) {
    return vsnprintf(str, (size_t)-1, fmt, args);
}