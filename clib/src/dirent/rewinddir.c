#include <dirent.h>
#include <syscall.h>
#include <errno.h>
#include "dirstream.h"

void rewinddir(DIR *dir) {
    if (!dir) {
        errno = EBADF;
        return;
    }

    dir->buf_pos = 0;
    dir->buf_end = 0;
    dir->tell = 0;

    syscall(SYS_lseek, dir->fd, 0, 0);
}
