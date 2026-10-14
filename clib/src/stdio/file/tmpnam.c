#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

static char __tmpnam_internal_buf[L_tmpnam];
static unsigned int __tmpnam_counter = 0;

char *tmpnam(char *buf) {
    char *target = buf ? buf : __tmpnam_internal_buf;
    pid_t pid = getpid();

    for (int attempts = 0; attempts < TMP_MAX; attempts++) {
        unsigned int count = __tmpnam_counter++;

        snprintf(
            target,
            L_tmpnam,
            "%s/t%04x%04x",
            P_tmpdir,
            (unsigned int)(pid & 0xffff),
            (unsigned int)(count & 0xffff)
        );

        int fd = open(target, O_RDONLY);
        if (fd < 0) {
            return target;
        }

        close(fd);
    }

    return NULL;
}
