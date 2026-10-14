#include "../internal.h"
#include <string.h>

struct string_view {
    const char *str;
    size_t len;
    size_t pos;
};

static int string_getc(void *ctx) {
    struct string_view *s = (struct string_view *)ctx;
    if (s->pos >= s->len) return EOF;
    return (unsigned char)s->str[s->pos++];
}

static int string_ungetc(int c, void *ctx) {
    struct string_view *s = (struct string_view *)ctx;
    if (c == EOF || s->pos == 0) return EOF;
    s->pos--;
    return (unsigned char)c;
}

int vsscanf(const char *restrict s, const char *restrict format, va_list arg) {
    struct string_view str = {
        .str = s,
        .len = strlen(s),
        .pos = 0
    };

    struct stream_source st = {
        .ctx = &str,
        .getc = string_getc,
        .ungetc = string_ungetc
    };

    return vscanfmt(&st, format, arg);
}