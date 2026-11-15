#include <sys/ioctl.h>
#include <syscall.h>
#include <stdarg.h>

int ioctl(int fd, unsigned long op, ...){
    va_list args;
    va_start(args, op);
    int ret = syscall(SYS_ioctl, fd, op, va_arg(args, void *));
    va_end(args);
    return ret;
}