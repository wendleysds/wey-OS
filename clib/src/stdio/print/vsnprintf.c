#include "../internal.h"

struct buffer_ctx {
    char *buf;
    size_t pos;
    size_t capacity;
};

static void buffer_putc(char c, void *ctx) {
    struct buffer_ctx *b = ctx;

    if (b->buf && b->pos < b->capacity) {
        b->buf[b->pos] = c;
    }

    b->pos++;
}

static void stream_putc(char c, void *ctx) {
    FILE *stream = ctx;
    fputc(c, stream);
}

int vsnprintf(char *restrict buf, size_t size, const char *restrict fmt, va_list args) {
    struct buffer_ctx ctx = {
        .buf = buf,
        .pos = 0,
        .capacity = (size > 0) ? (size - 1) : 0
    };

    vprintfmt(buffer_putc, &ctx, fmt, args);

    if (buf && size > 0) {
        size_t nul_pos = (ctx.pos < size) ? ctx.pos : (size - 1);
        buf[nul_pos] = '\0';
    }

    return (int)ctx.pos;
}