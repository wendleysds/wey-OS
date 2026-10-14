#include <stdio.h>
#include <fcntl.h>

FILE *tmpfile(void){
    int fd = open("/tmp", O_RDWR | O_TMPFILE | O_EXCL, 0600);

    // fallback with mkstemp + unlink

    if (fd < 0) {
        return NULL;
    }

    return fdopen(fd, "wb+");
}