#ifndef _SCHED_TASK_H
#define _SCHED_TASK_H

#include <wey/sched.h>

struct task;

extern void free_task(struct task *task);
extern struct task* alloc_task();

extern pid_t kernel_thread(int (*fn)(), const char* name);

#endif