#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>

int vsnprintf(char* restrict buf, unsigned int size, const char* restrict fmt, va_list args);
int snprintf(char* restrict buf, unsigned int size, const char* restrict fmt, ...);

#endif
