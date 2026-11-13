#ifndef _KERNEL_SIGNAL_H
#define _KERNEL_SIGNAL_H

#include <def/bits.h>
#include <sys/types.h>
#include <asm/ptrace.h>
#include <stdbool.h>

struct task;

typedef void (*sighandler_t)(int);
typedef void (*sigrestore_t)(void);

#define NSIG 32

#define	SIG_ERR ((sighandler_t) -1)
#define	SIG_DFL ((sighandler_t)  0)
#define	SIG_IGN ((sighandler_t)  1)

#define	SIG_BLOCK   0
#define	SIG_UNBLOCK 1	 
#define	SIG_SETMASK 2

#define	SA_NOCLDSTOP  BIT(0)
#define SA_NOCLDWAIT  BIT(1)
#define SA_SIGINFO    BIT(2)
#define SA_ONSTACK    BIT(3)
#define SA_RESTART    BIT(4)
#define SA_NODEFER    BIT(5)
#define SA_RESETHAND  BIT(6)

#define SIGHUP      1
#define SIGINT      2
#define SIGQUIT     3
#define SIGILL      4
#define SIGTRAP     5
#define SIGABRT     6
#define SIGIOT      SIGABRT
#define SIGBUS      7
#define SIGFPE      8
#define SIGKILL     9
#define SIGUSR1    10
#define SIGSEGV    11
#define SIGUSR2    12
#define SIGPIPE    13
#define SIGALRM    14
#define SIGTERM    15
#define SIGSTKFLT  16
#define SIGCHLD    17
#define SIGCONT    18
#define SIGSTOP    19
#define SIGTSTP    20
#define SIGTTIN    21
#define SIGTTOU    22
#define SIGURG     23
#define SIGXCPU    24
#define SIGXFSZ    25
#define SIGVTALRM  26
#define SIGPROF    27
#define SIGWINCH   28
#define SIGIO      29
#define SIGPOLL    SIGIO
#define SIGPWR     30
#define SIGSYS     31
#define SIGUNUSED  31

#define SIGMASK(s)       BIT((s) - 1)
#define SIG_UNBLOCKABLE  (SIGMASK(SIGKILL) | SIGMASK(SIGSTOP))

struct sigaction {
	sighandler_t sa_handler;
	sigset_t sa_mask;
	int sa_flags;
	sigrestore_t sa_restorer;
};

asmlinkage void do_signal(struct registers *regs);
bool has_signal_pending(struct task *task);
int send_signal(struct task *task, int sig);
size_t send_signal_to_process_group(pid_t pgid, int sig);

#endif /* _KERNEL_SIGNAL_H */