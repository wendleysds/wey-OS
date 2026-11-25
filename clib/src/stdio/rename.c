#include <stdio.h>
#include <syscall.h>

int rename(const char *oldpath, const char *newpath) {
    return syscall(SYS_rename, oldpath, newpath);
}