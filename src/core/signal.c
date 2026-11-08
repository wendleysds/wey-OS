#include <kernel/sched.h>
#include <kernel/signal.h>
#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <def/errno.h>
#include <lib/assert.h>
#include <asm/signal.h>

static bool default_is_ignore(int sig) {
    switch(sig){
        case SIGCHLD:
        case SIGURG:
        case SIGWINCH:
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
            return true;
        default:
            return false;
    }
}

bool has_signal_pending(struct task *task){
	return task->sig_pending & ~task->sig_blocked;
}

asmlinkage void do_signal(struct registers *regs) {
    if(!regs_is_user_mode(regs)) return;
    struct task* cur = current;

    int status = 0;
    bool core_dump = false;

    sigset_t ready = cur->sig_pending & ~cur->sig_blocked;
    if (!ready) return;

    for (int bit = 0; bit < 31; bit++) {
        if (BIT_CHECK(ready, bit)) {
            cur->sig_pending = BIT_CLEAR(cur->sig_pending, bit);
            int signo = bit + 1;

            struct sigaction *act = &cur->sig_actions[signo];
            if (act->sa_handler == SIG_IGN) return;

            if (act->sa_handler == SIG_DFL) {
                if (cur->pid == 1 || default_is_ignore(signo)) continue;
                status = signo;
                goto exit_task;
            }

            if (arch_setup_signal_frame(signo, act, regs) < 0) {
                status = SIGSEGV;
                goto exit_task;
            }

            cur->sig_blocked |= act->sa_mask;
            if (!(act->sa_flags & SA_NODEFER)) {
                cur->sig_blocked |= SIGMASK(signo);
            }

            return;
        }
    }

exit_task:
    task_exit(cur, TASK_STATUS_ENCODE_SIGNAL(status, core_dump));
    schedule();
    unreachable();
}

int send_signal(struct task *task, int sig) {
    if (sig < 0 || sig >= NSIG) return -EINVAL;
    if (sig == 0 || task->state == TASK_ZOMBIE) return 0;

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
