#include <unistd.h>
#include <syscall.h>

int pipe2(int pipefd[2], int flags){
    return syscall(SYS_pipe2, pipefd, flags);
}