#ifndef _CPU_SCHEDULER_H
#define _CPU_SCHEDULER_H

#include <asm/ptrace.h>
#include <asm/process.h>
#include <wey/vma.h>
#include <wey/pid.h>
#include <def/config.h>
#include <def/init.h>
#include <lib/list.h>

typedef enum {
	TASK_NEW,
	TASK_READY,
	TASK_RUNNING,
	TASK_ZOMBIE,
	TASK_BLOCKED,
} task_state_t;

struct task {
	pid_t pid;
	struct registers regs;

	char name[PROC_NAME_MAX];
	struct file* file_table[PROC_FD_MAX];	
	void* kstack;
	
	struct list_head tasks;
	struct list_head queue;

	struct mm_struct* mm;
	char *pwd;

	task_state_t state;
	int priority;
	int exit_code;

	struct task* parent;

	struct list_head children;
	struct list_head sibling;
};

struct task* task_create(const char* name, int priority);
void task_exit(struct task* task, int status);
void task_destroy(struct task* task);

struct task* task_get_child(struct task* parent, pid_t pid);
struct task* task_find_zombie_child(struct task* parent);
void task_reparent_children(struct task* task, struct task* new_parent);

asmlinkage void task_sleep(struct task* task);
asmlinkage void task_wakeup(struct task* task);

asmlinkage void schedule();

int __init scheduler_init();
void __init scheduler_start();
void scheduler_add(struct task* task);
void scheduler_remove(struct task* task);

static inline void sleep_current(){
	task_sleep(current);
	schedule();
}

#endif
