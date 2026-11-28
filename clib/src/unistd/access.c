#include <unistd.h>
#include <syscall.h>

int access(const char *pathname, int mode) {
    return syscall(SYS_access, pathname, mode);
}