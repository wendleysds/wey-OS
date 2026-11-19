#include <unistd.h>
#include <syscall.h>

int dup2(int oldfd, int newfd){
	return syscall(SYS_dup2, oldfd, newfd);
}

