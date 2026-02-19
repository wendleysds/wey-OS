#ifndef _SCHED_TASK_H
#define _SCHED_TASK_H

#include <wey/sched.h>

struct task;

struct task* task_create(const char* name, int priority);
int task_destroy(struct task* task);

pid_t kernel_thread(int (*fn)(), const char* name);

#endif