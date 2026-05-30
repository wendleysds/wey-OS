#include <wey/syscall.h>
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

SYSCALL_DEFINE0(getpid){
	return current->pid;
}

SYSCALL_DEFINE3(waitpid, pid_t, pid, int*, wstatus, int, options){
	for (;;) {
		struct task* child = NULL;
	
		if(pid > 0){
			child = task_get_child(current, pid);
			if(!child){
				return NOT_FOUND;
			}
		}else{
			child = task_find_zombie_child(current);
		}

		if(child){
			if(child->state == TASK_ZOMBIE){
				if(wstatus){
					*wstatus = child->exit_code;
				}

				pid_t child_pid = child->pid;
				task_destroy(child);
				return child_pid;
			}
		}

		if(options & WNOHANG){
			return 0;
		}

		sleep_current();
	}
}