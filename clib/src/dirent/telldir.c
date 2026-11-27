#include <dirent.h>
#include <errno.h>
#include "dirstream.h"

long telldir(DIR *dir) {
    if (!dir) {
        errno = EBADF;
        return -1;
    }

    return (long)dir->tell;
}
