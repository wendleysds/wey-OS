#ifndef _CLIB_STDIO_INTERNAL_H
#define _CLIB_STDIO_INTERNAL_H

#include <stdio.h>
#include <stdint.h>

#define _FILE_READ   (1 << 0)
#define _FILE_WRITE  (1 << 1)
#define _FILE_EOF    (1 << 2)
#define _FILE_ERR    (1 << 3)
#define _FILE_DIRTY  (1 << 4)
#define _FILE_UNBUF  (1 << 5)

struct file {
    int fd;
    char* buffer;
    size_t capacity;
    size_t size;
    size_t pos;
    int flags;
    int ungetc_buf; // -1 == empty, else == character
};

#endif