#include "wey/spinlock.h"
#include <wey/sched.h>
#include <def/config.h>
#include <def/status.h>

#define PID_FREE 0
#define PID_ALLOC 1

static pid_t pids[PROC_MAX];
static pid_t next_free_pid;
static spinlock_t pid_spinlock;

pid_t pid_alloc(){
	spin_lock(&pid_spinlock);
	pid_t start = next_free_pid;
	pid_t i = start;

	do {
		if(pids[i] == PID_FREE){
			pids[i] = PID_ALLOC;
			next_free_pid = (i + 1) % PROC_MAX;
			spin_unlock(&pid_spinlock);
			return i;
		}
		i = (i + 1) % PROC_MAX;
	} while(i != start);

	spin_unlock(&pid_spinlock);
	return LIST_FULL;
}

void pid_free(pid_t pid){
	pids[pid] = PID_FREE;
	next_free_pid = pid;
}

static int __init pid_init(){
	for(pid_t i = 0; i < PROC_MAX; i++){
		pids[i] = PID_FREE;
	}

	next_free_pid = pids[0] = PID_ALLOC; // reserved init

	spinlock_init(&pid_spinlock);
	return OK;
}

core_initcall(pid_init);
