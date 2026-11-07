#include <signal.h>
#include <syscall.h>

extern void __sigreturn_trampoline(void);

int sigaction(int signum, const struct sigaction *restrict act, struct sigaction *restrict old) {
	struct sigaction k;
    if (act) {
        k = *act;
        k.sa_restorer = __sigreturn_trampoline;
        act = &k;
    }

	return syscall(SYS_sigaction, signum, act, old);
}