#include <wey/sched.h>
#include <def/config.h>
#include <def/status.h>

#define PID_START 1 // pid 0 reserved for idle task
#define PID_FREE -1

static pid_t pids[PROC_MAX];
static pid_t next_free_pid;
static spinlock_t pid_spinlock;

pid_t pid_alloc(){
try_again:
	spin_lock(&pid_spinlock);
	pid_t pid_end = PROC_MAX;
	for(pid_t i = next_free_pid; i < pid_end; i++){
		if(pids[i] == PID_FREE){
			pids[i] = i;
			spin_unlock(&pid_spinlock);
			return i;
		}
	}

	spin_unlock(&pid_spinlock);

	if(next_free_pid != PID_START){
		pid_end = next_free_pid;
		next_free_pid = PID_START;
		goto try_again;
	}

	return LIST_FULL;
}

void pid_free(pid_t pid){
	pids[pid] = PID_FREE;
	next_free_pid = pid;
}

void __init pid_init(){
	for(pid_t i = 1; i < PROC_MAX; i++){
		pids[i] = PID_FREE;
	}

	next_free_pid = PID_START;
	spinlock_init(&pid_spinlock);
}
