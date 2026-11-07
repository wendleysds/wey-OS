#include <kernel/sched.h>
#include <kernel/signal.h>
#include <kernel/uaccess.h>
#include <kernel/syscall.h>
#include <def/errno.h>
#include <lib/assert.h>
#include <def/config.h>

#include <asm/ptrace.h>
#include <asm/signal.h>
#include <asm/cpuflags.h>

struct sigframe {
    sigrestore_t restore;
    int sig;
    struct registers saved_regs;
    sigset_t old_mask;
};

int arch_setup_signal_frame(int sig, const struct sigaction *act, struct registers *regs) {
    uintptr_t sp = regs->sp;
    sp -= sizeof(struct sigframe);
    sp &= -16;

    struct task* cur = current;
    struct sigframe frame;
    frame.restore = act->sa_restorer;
    frame.sig = sig;
    frame.saved_regs = *regs;
    frame.old_mask = cur->sig_blocked;

    if (copy_to_user((void*)sp, &frame, sizeof(frame)) != 0) {
        return -EFAULT;
    }

    regs->sp = sp;
    regs->ip = (uintptr_t)act->sa_handler;

    return 0;
}

SYSCALL_DEFINE0(sigreturn) {
    struct registers *regs = &current->regs;

    struct sigframe frame;
    if (copy_from_user(&frame, (void __user *)(regs->sp - 4), sizeof(frame))) {
		task_exit(current, SIGSEGV);
        schedule();
        unreachable();
	}

    current->regs = frame.saved_regs;
    current->sig_blocked = frame.old_mask & ~SIG_UNBLOCKABLE;

    regs->cs = GDT_USER_CODE;
    regs->ss = GDT_USER_DATA;
    regs->flags |= X86_EFLAGS_IF;

    return frame.saved_regs.ax;
}

