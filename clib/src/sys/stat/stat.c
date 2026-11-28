#include <sys/stat.h>
#include <syscall.h>

int stat(const char *restrict pathname, struct stat *restrict statbuf){
    return syscall(SYS_stat, pathname, statbuf);
}