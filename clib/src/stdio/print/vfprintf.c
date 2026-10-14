#include "../internal.h"

static void stream_putc(char c, void *ctx) {
    FILE *stream = ctx;
    fputc(c, stream);
}

int vfprintf(FILE *restrict stream, const char *restrict fmt, va_list args) {
    if (!stream) {
        return -1;
    }

    int count = vprintfmt(stream_putc, stream, fmt, args);
    
    if (ferror(stream)) {
        count = -1;
    }

    return count;
}