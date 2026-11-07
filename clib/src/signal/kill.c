#include <signal.h>
#include <syscall.h>

int kill(pid_t pid, int signum){
	return syscall(SYS_kill, pid, signum);
}