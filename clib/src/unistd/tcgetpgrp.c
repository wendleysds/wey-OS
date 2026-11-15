#include <sys/ioctl.h>
#include <sys/ioctls.h>
#include <unistd.h>

pid_t tcgetpgrp(int fd){
    pid_t pgid;
    if (ioctl(fd, TIOCGPGRP, &pgid) == -1) {
        return -1;
    }
    return pgid;
}
