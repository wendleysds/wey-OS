#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "dirstream.h"

int closedir(DIR *dir) {
    if (!dir) {
        errno = EBADF;
        return -1;
    }

    int fd = dir->fd;
    free(dir);
    return close(fd);
}
