#include "../internal.h"

static int file_getc(void *ctx) {
    return fgetc((FILE *)ctx);
}

static int file_ungetc(int c, void *ctx) {
    return ungetc(c, (FILE *)ctx);
}

int vfscanf(FILE *restrict stream, const char *restrict format, va_list arg) {
    struct stream_source s = {
        .ctx = stream,
        .getc = file_getc,
        .ungetc = file_ungetc
    };
    return vscanfmt(&s, format, arg);
}