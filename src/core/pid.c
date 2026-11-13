#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <sync/spinlock.h>
#include <def/errno.h>
#include <uapi/sys/wait.h>

#define PID_FREE 0
#define PID_ALLOC 1

static pid_t pids[PROC_MAX] = { [0] = PID_ALLOC };
static pid_t next_free_pid = 1;

static DEFINE_SPINLOCK(pid_spinlock);

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
	return -ENOENT;
}

void pid_free(pid_t pid){
	if(pid <= 0 || pid >= PROC_MAX){
		return;
	}

	spin_lock(&pid_spinlock);
	pids[pid] = PID_FREE;
	next_free_pid = pid;
	spin_unlock(&pid_spinlock);
}

SYSCALL_DEFINE0(getpid){
	return current->pid;
}

SYSCALL_DEFINE3(waitpid, pid_t, pid, int*, wstatus, int, options){
	for (;;) {
		struct task* child = NULL;
	
		if(pid > 0){
			child = task_get_child(current, pid);
			if(!child){
				return -ECHILD;
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

		if(has_signal_pending(current)){
			return -EINTR;
		}

		sleep_interruptible();
	}
}
