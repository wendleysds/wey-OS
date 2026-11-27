#ifndef _DIRSTREAM_H
#define _DIRSTREAM_H

#include <dirent.h>
#include <sys/types.h>
#include <stddef.h>

#define DIR_BUF_SIZE 2048

struct __dirstream {
    int fd;
    size_t buf_pos;
    size_t buf_end;
    off_t tell;
    struct dirent dent;
    char buf[DIR_BUF_SIZE];
};

#endif /* _DIRSTREAM_H */
