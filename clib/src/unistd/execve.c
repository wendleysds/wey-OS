#include <unistd.h>
#include <syscall.h>

int execve(const char *filename, char *const argv[], char *const envp[]) {
    return syscall(SYS_execve, filename, argv, envp);
}
