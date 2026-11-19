#include <unistd.h>
#include <syscall.h>

int pipe(int pipefd[2]){
	return syscall(SYS_pipe, pipefd);
}
