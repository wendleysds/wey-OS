#include <sys/ioctl.h>
#include <sys/ioctls.h>
#include <unistd.h>

int tcsetpgrp(int fd, pid_t pgrp) {
    if (ioctl(fd, TIOCSPGRP, &pgrp) == -1) {
        return -1;
    }
    return 0;
}