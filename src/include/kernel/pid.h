#ifndef _PID_H
#define _PID_H

#include <sys/types.h>

#define WNOHANG 0x1

pid_t pid_alloc();
void pid_free(pid_t pid);

#endif
