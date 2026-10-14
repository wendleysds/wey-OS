#include <syscall.h>
#include <stdarg.h>
#include <fcntl.h>

int open(const char *restrict path, int flags, ...){
    int mode = 0;
    if ((flags & O_CREAT) || (flags & O_EXCL)) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, int);
        va_end(args);
    }

    return syscall(SYS_open, path, flags, mode);
}