#include <unistd.h>
#include <syscall.h>

int truncate(const char* filename, off_t length){
    return syscall(SYS_truncate, filename, length);
}
    