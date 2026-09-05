#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>

int vprintfmt(void (*putc)(char c, void *ctx), void *ctx, const char *fmt, va_list args);
int vsnprintf(char* restrict buf, unsigned int size, const char* restrict fmt, va_list args);
int snprintf(char* restrict buf, unsigned int size, const char* restrict fmt, ...);

#endif
