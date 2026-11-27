#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>

DIR *opendir(const char *path) {
    int fd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) {
        return NULL;
    }

    DIR *dir = fdopendir(fd);
    if (!dir) {
        close(fd);
        return NULL;
    }

    return dir;
}