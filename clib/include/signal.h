#ifndef _CLIB_SIGNAL_H
#define _CLIB_SIGNAL_H

#define SIGABRT 0
#define SIGALRM 1
#define SIGBUS  2
#define SIGCHLD 3
#define SIGCONT 4
#define SIGFPE  5
#define SIGHUP  6
#define SIGILL  7
#define SIGINT  8
#define SIGKILL 9
#define SIGPIPE 10
#define SIGQUIT 11
#define SIGSEGV 12
#define SIGSTOP 13
#define SIGTERM 14
#define SIGTSTP 15
#define SIGTTIN 16
#define SIGTTOU 17
#define SIGUSR1 18
#define SIGUSR2 19
#define SIGPOLL 20
#define SIGPROF 21
#define SIGSYS  22
#define SIGTRAP 23
#define SIGURG  24
#define SIGVTALRM 25
#define SIGXCPU 26
#define SIGXFSZ 27

typedef int sig_atomic_t;

typedef void (*sighandler_t)(int);

#define SIG_DFL ((sighandler_t)-1)
#define SIG_ERR ((sighandler_t)-2)
#define SIG_HOLD ((sighandler_t)-3)
#define SIG_IGN ((sighandler_t)-4)

sighandler_t signal(int signum, sighandler_t handler);


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