#include <unistd.h>
#include <syscall.h>
#include <stdarg.h>
#include <fcntl.h>

int open(const char *restrict path, int flags, ...){
    int mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, int);
        va_end(args);
    }

    return syscall(SYS_open, (long)path, flags, mode);
}

int close(int fd){
    return syscall(SYS_close, fd);
}

ssize_t read(int fd, void* buffer, size_t size){
    return syscall(SYS_read, fd, (long)buffer, size);
}

ssize_t write(int fd, const void* buffer, size_t size){
    return syscall(SYS_write, fd, (long)buffer, size);
}

off_t lseek(int fd, off_t offset, int whence){
    return syscall(SYS_lseek, fd, offset, whence);
}

pid_t getpid(void) {
    return syscall(SYS_getpid);
}