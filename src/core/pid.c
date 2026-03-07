#include "wey/spinlock.h"
#include <wey/sched.h>
#include <def/config.h>

static pid_t pids[PROC_MAX];
static pid_t next_free_pid;
static spinlock_t pid_spinlock;

pid_t pid_alloc(){
try_again:
	spin_lock(&pid_spinlock);
	pid_t pid_end = PROC_MAX;
	for(pid_t i = next_free_pid; i < pid_end; i++){
		if(pids[i] == -1){
			pids[i] = i;
			spin_unlock(&pid_spinlock);
			return i;
		}
	}

	spin_unlock(&pid_spinlock);

	if(next_free_pid != 0){
		pid_end = next_free_pid;
		next_free_pid = 0;
		goto try_again;
	}

	return -1;
}

void pid_free(pid_t pid){
	pids[pid] = -1;
	next_free_pid = pid;
}

void __init pid_init(){
	for(pid_t i = 0; i < PROC_MAX; i++){
		pids[i] = -1;
	}

	next_free_pid = 0;
	spinlock_init(&pid_spinlock);
}
