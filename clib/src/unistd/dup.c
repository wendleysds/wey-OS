#include <unistd.h>
#include <syscall.h>

int dup(int oldfd){
	return syscall(SYS_dup, oldfd);
}