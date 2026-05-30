#ifndef _PID_H
#define _PID_H

#define WNOHANG    0x1

typedef int pid_t;

pid_t pid_alloc();
void pid_free(pid_t pid);

#endif
