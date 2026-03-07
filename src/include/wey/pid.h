#ifndef _PID_H
#define _PID_H

typedef short pid_t;

pid_t pid_alloc();
void pid_free(pid_t pid);
void pid_init();

#endif
