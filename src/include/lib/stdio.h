#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>

int vsprintf(char* restrict buf, const char* fmt, va_list args);
int vsnprintf(char* restrict buf, unsigned int size, const char* restrict fmt, va_list args);
int sprintf(char* restrict buf, const char* restrict fmt, ...);
int snprintf(char* restrict buf, unsigned int size, const char* restrict fmt, ...);

#endif
