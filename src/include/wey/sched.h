#ifndef _CPU_SCHEDULER_H
#define _CPU_SCHEDULER_H

#include <asm/ptrace.h>
#include <wey/mm_types.h>
#include <def/compile.h>
#include <def/config.h>
#include <def/init.h>
#include <lib/list.h>

typedef unsigned short pid_t;

typedef enum {
	TASK_NEW,
	TASK_READY,
	TASK_RUNNING,
	TASK_FINISHED,
	TASK_SLEEPING
} task_state_t;

struct task {
	pid_t pid;

	char name[PROC_NAME_MAX];
	int fileDescriptors[PROC_FD_MAX];

	struct registers regs;
	
	void* ustack;
	void* kstack;
	
	struct list_head tasks;
	
	struct mm_struct* mm;
	task_state_t state;
	int priority;
	char *pwd;

	int exit_state;
	int exit_code;

	struct task* parent;

	struct list_head children;
	struct list_head sibling;
};

asmlinkage void schedule();

int __init scheduler_init();
void scheduler_add(struct task* task);
void scheduler_remove(struct task* task);

#endif
