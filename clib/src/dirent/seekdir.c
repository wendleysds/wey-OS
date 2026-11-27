#include <dirent.h>
#include <syscall.h>
#include <errno.h>
#include "dirstream.h"

void seekdir(DIR *dir, long offset) {
    if (!dir || offset < 0) {
        return;
    }

    dir->buf_pos = 0;
    dir->buf_end = 0;
    dir->tell = offset;

    syscall(SYS_lseek, dir->fd, (off_t)offset, 0);
}
