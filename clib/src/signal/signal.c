#include <signal.h>

sighandler_t signal(int signum, sighandler_t handler) {
    struct sigaction act = { .sa_handler = handler }, old;
	return sigaction(signum, &act, &old) < 0 ? SIG_ERR : old.sa_handler;
}