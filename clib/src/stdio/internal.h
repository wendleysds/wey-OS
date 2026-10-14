#ifndef _CLIB_STDIO_INTERNAL_H
#define _CLIB_STDIO_INTERNAL_H

#include <stdio.h>

#define _FILE_READ    (1 << 0)
#define _FILE_WRITE   (1 << 1)
#define _FILE_EOF     (1 << 2)
#define _FILE_ERR     (1 << 3)
#define _FILE_DIRTY   (1 << 4)
#define _FILE_UNBUF   (1 << 5)
#define _FILE_LINEBUF (1 << 6)
#define _FILE_OWNBUF  (1 << 7)

struct file {
    int fd;
    char* buffer;
    size_t capacity;
    size_t size;
    size_t pos;
    int flags;
    int ungetc_buf; // -1 == empty, else == character
};

struct stream_source {
    void *ctx;
    int (*getc)(void *);
    int (*ungetc)(int, void *);
};

int vprintfmt(void (*putc)(char c, void *ctx), void *ctx, const char *fmt, va_list args);
int vscanfmt(struct stream_source *restrict stream, const char *restrict fmt, va_list ap);

int __mode_to_open_flags(const char *mode);
int __mode_to_file_flags(const char *mode);
int __file_register(struct file *stream);

void *malloc(size_t size);
void free(void *ptr);

#endif