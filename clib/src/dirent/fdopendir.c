#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include "dirstream.h"

DIR *fdopendir(int fd) {
    if (fd < 0) {
        errno = EBADF;
        return NULL;
    }

    DIR *dir = (DIR *)malloc(sizeof(DIR));
    if (!dir) {
        errno = ENOMEM;
        return NULL;
    }

    dir->fd = fd;
    dir->buf_pos = 0;
    dir->buf_end = 0;
    dir->tell = 0;

    return dir;
}

int dirfd(DIR *dir) {
    if (!dir) {
        errno = EINVAL;
        return -1;
    }
    return dir->fd;
}