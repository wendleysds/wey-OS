#include <unistd.h>
#include <syscall.h>

int ftruncate(int fd, off_t length){
    return syscall(SYS_ftruncate, fd, length);
}