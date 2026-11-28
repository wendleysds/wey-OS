#include <kernel/sched.h>
#include <kernel/signal.h>
#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <def/errno.h>
#include <lib/assert.h>
#include <asm/signal.h>

#define STOP_MASK (SIGMASK(SIGSTOP)|SIGMASK(SIGTSTP)|SIGMASK(SIGTTIN)|SIGMASK(SIGTTOU))

static void notify_parent(struct task *t, bool jobctl) {
	struct task *p = t->parent;
	if (!p) return;

	if (!(jobctl && (p->sig_actions[SIGCHLD].sa_flags & SA_NOCLDSTOP)))
		send_signal(p, SIGCHLD);

	wake_up_all(&p->child_wait);
}

static bool default_is_ignore(int sig) {
    switch(sig){
        case SIGCHLD:
        case SIGURG:
        case SIGWINCH:
        case SIGCONT:
            return true;
        default:
            return false;
    }
}

static bool default_is_core_dump(int sig){
    switch(sig){
        case SIGBUS:
        case SIGABRT:
        case SIGILL:
        case SIGQUIT:
        case SIGSEGV:
        case SIGSYS: // or SIGUNUSED
        case SIGTRAP:
        case SIGXCPU:
        case SIGXFSZ:
            return true;
        default:
            return false;
    }
}

static bool default_is_stop(int sig){
    switch(sig){
        case SIGSTOP:
        case SIGTSTP:
        case SIGTTIN:
        case SIGTTOU:
            return true;
        default:
            return false;
    }
}

bool has_signal_pending(struct task *task){
	return task->sig_pending & ~task->sig_blocked;
}

static __no_return void die_by_signal(struct task *t, int sig) {
	task_exit(t, TASK_STATUS_ENCODE_SIGNAL(sig, default_is_core_dump(sig)));
	schedule();
	unreachable();
}

asmlinkage void do_signal(struct registers *regs) {
	if (!regs_is_user_mode(regs)) return;
	struct task *cur = current;

	for (;;) {
		sigset_t ready = cur->sig_pending & ~cur->sig_blocked;
		if (!ready) return;

		int signo = __builtin_ctz(ready) + 1;
		cur->sig_pending &= ~SIGMASK(signo);

		struct sigaction *act = &cur->sig_actions[signo];

		if (act->sa_handler == SIG_IGN) continue;

		if (act->sa_handler == SIG_DFL) {
			if (default_is_ignore(signo)) continue;

			if (default_is_stop(signo)) {
				if (cur->pid == 1) continue;

				cur->stop_sig = signo;
                cur->stop_reported = false;
                cur->continued = false; 
				cur->state = TASK_STOPPED;

				notify_parent(cur, true);
				schedule();
				continue;
			}

			die_by_signal(cur, signo);
		}

		if (arch_setup_signal_frame(signo, act, regs) < 0)
			die_by_signal(cur, SIGSEGV);

		cur->sig_blocked |= act->sa_mask;
		if (!(act->sa_flags & SA_NODEFER))
			cur->sig_blocked |= SIGMASK(signo);
		return;
	}
}

int send_signal(struct task *task, int sig) {
    if (sig < 0 || sig >= NSIG) return -EINVAL;
    if (sig == 0 || task->state == TASK_ZOMBIE) return 0;

    if (sig == SIGCONT) {
        task->sig_pending &= ~STOP_MASK;
        if (task->state == TASK_STOPPED) {
            task->state = TASK_READY;
            task->continued = true;
            scheduler_add(task);
            notify_parent(task, true);
        }
    } else if (STOP_MASK & SIGMASK(sig)) {
        task->sig_pending &= ~SIGMASK(SIGCONT);
    }

    if (sig == SIGKILL && task->state == TASK_STOPPED) {
        task->state = TASK_READY;
        scheduler_add(task);
    }

	sighandler_t handler = task->sig_actions[sig].sa_handler;
	if (sig != SIGKILL && (handler == SIG_IGN || (handler == SIG_DFL && default_is_ignore(sig))))
		return 0;

	task->sig_pending |= SIGMASK(sig);
	if (task->state == TASK_INTERRUPTIBLE && !(task->sig_blocked & SIGMASK(sig)))
		task_wakeup(task);

	return 0;
}

SYSCALL_DEFINE3(sigaction,
    int, signum,
    const struct sigaction *restrict, act,
    struct sigaction *restrict, oldact
) {
    struct task* cur = current;

    if (signum < 0 || signum >= NSIG) return -EINVAL;
    if (signum == 0) return -EINVAL;
    if (signum == SIGKILL || signum == SIGSTOP) return -EINVAL;

    if (oldact) {
        if (copy_to_user(oldact, &cur->sig_actions[signum], sizeof(struct sigaction))) {
            return -EFAULT;
        }
    }

    if (act) {
        if (copy_from_user(&cur->sig_actions[signum], act, sizeof(struct sigaction))) {
            return -EFAULT;
        }
    }

    return 0;
}
