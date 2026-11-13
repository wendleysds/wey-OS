#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include_next <sys/wait.h>

#include <sys/types.h>

pid_t waitpid(pid_t pid, int *wstatus, int options);

#endif /*_SYS_WAIT_H*/