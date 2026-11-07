#ifndef _CLIB_SIGNAL_H
#define _CLIB_SIGNAL_H

typedef int sig_atomic_t;

#include <sys/types.h>

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

struct sigaction {
	sighandler_t sa_handler;
	sigset_t sa_mask;
	int sa_flags;
	sigrestore_t sa_restorer;
};

sighandler_t signal(int signum, sighandler_t handler);
int sigaction(int signum, const struct sigaction *restrict act, struct sigaction *restrict old);

// sighs and signals:
//     kill      send a signal to a process (or group of processes)
//     killpg    send a signal to a process group
//     psiginfo  print signal information (detailed)
//     psignal   print a signal message (simple)
//     pthread_kill  send a signal to a thread
//     pthread_sigmask  manage signal mask (threads)
//     raise     send a signal to the calling process (self)
//     sigaction  set a signal handler (advanced)
//     sigaddset  add a signal to a set
//     sigaltstack  set an alternate stack for signals
//     sigdelset  delete a signal from a set
//     sigemptyset  clear a signal set
//     sigfillset  fill a signal set (all signals)
//     sighold     hold (block) a signal
//     sigignore   ignore a signal (obsolete)
//     siginterrupt  change signal behavior on interrupt
//     sigismember  check if a signal is in a set
//     signal      set a signal handler (simple)
//     sigpause    wait for signal (replaces signal)
//     sigpending  get pending signals
//     sigprocmask  manage signal mask (process)
//     sigqueue    send a signal with data
//     sigrelse    release (unblock) a signal
//     sigset      set a signal handler (replaces signal)
//     sigsuspend  wait for signal (replaces signal)
//     sigtimedwait  wait for signal with timeout
//     sigwait     wait for signal (replaces signal)
//     sigwaitinfo wait for signal with info

/*
int kill(pid_t, int);
int killpg(pid_t, int);
voidpsiginfo(const siginfo_t *, const char *);
voidpsignal(int, const char *);
int pthread_kill(pthread_t, int);
int pthread_sigmask(int, const sigset_t *restrict, sigset_t *restrict);
int raise(int);
int sigaction(int, const struct sigaction *restrict, struct sigaction *restrict);
int sigaddset(sigset_t *, int);
int sigaltstack(const stack_t *restrict, stack_t *restrict);
int sigdelset(sigset_t *, int);
int sigemptyset(sigset_t *);
int sigfillset(sigset_t *);
int sighold(int);
int sigignore(int);
int siginterrupt(int, int);
int sigismember(const sigset_t *, int);
void (*signal(int, void (*)(int)))(int);
int sigpause(int);
int sigpending(sigset_t *);
int sigprocmask(int, const sigset_t *restrict, sigset_t *restrict);
int sigqueue(pid_t, int, union sigval);
int sigrelse(int);
void (*sigset(int, void (*)(int)))(int);
int sigsuspend(const sigset_t *);
int sigtimedwait(const sigset_t *restrict, siginfo_t *restrict, const struct timespec *restrict);
int sigwait(const sigset_t *restrict, int *restrict);
int sigwaitinfo(const sigset_t *restrict, siginfo_t *restrict);
*/

#endif /* _CLIB_SIGNAL_H */