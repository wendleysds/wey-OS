#include <unistd.h>
#include <syscall.h>

ssize_t write(int fd, const void* buffer, size_t size){
    return syscall(SYS_write, fd, (long)buffer, size);
}
