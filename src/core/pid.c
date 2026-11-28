#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/uaccess.h>
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

static bool child_matches(struct task *me, struct task *c, pid_t pid) {
	if (pid > 0)  return c->pid == pid;
	if (pid == -1) return true;
	if (pid == 0) return c->pgid == me->pgid;
	return c->pgid == -pid;
}

SYSCALL_DEFINE0(getpid){
	return current->pid;
}

SYSCALL_DEFINE3(waitpid, pid_t, pid, int __user *, wstatus, int, options) {
	if (options & ~(WNOHANG | WUNTRACED | WCONTINUED))
		return -EINVAL;

	struct task *me = current;
	struct wait_queue_entry wait;
	long ret;

	wait_queue_entry_init(&wait, me, task_default_wakeup);
	wait_queue_add(&me->child_wait, &wait);

	for (;;) {
		struct task *c, *hit = NULL;
		bool have_child = false, reap = false;
		int status = 0;

		list_for_each_entry(c, &me->children, sibling) {
			if (!child_matches(me, c, pid)) continue;
			have_child = true;

			if (c->state == TASK_ZOMBIE) {
				hit = c; reap = true; status = c->exit_code;
				break;
			}
			if (c->state == TASK_STOPPED && (options & WUNTRACED) && !c->stop_reported) {
				hit = c;
				status = TASK_STATUS_ENCODE_STOPPED(c->stop_sig);
				break;
			}

			if ((options & WCONTINUED) && c->continued) {
				hit = c;
				status = TASK_EXIT_CODE_CONTINUED;
				break;
			}
		}

		if (hit) {
			if (wstatus && copy_to_user(wstatus, &status, sizeof(status))) {
				ret = -EFAULT;
				break;
			}
			ret = hit->pid;
			if (reap) {
				task_destroy(hit);
			} else if (status == TASK_EXIT_CODE_CONTINUED) {
				hit->continued = false;
			} else {
				hit->stop_reported = true;
			}

			break;
		}

		if (!have_child) {
			ret = -ECHILD;
			break;
		}
		
		if (options & WNOHANG) {
			ret = 0;
			break;
		}

		if (has_signal_pending(me)) {
			ret = -EINTR;
			break;
		}

		sleep_interruptible();
	}

	wait_queue_remove(&me->child_wait, &wait);
	return ret;
}