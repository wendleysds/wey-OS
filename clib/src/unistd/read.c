#include <unistd.h>
#include <syscall.h>

ssize_t read(int fd, void* buffer, size_t size){
    return syscall(SYS_read, fd, buffer, size);
}
