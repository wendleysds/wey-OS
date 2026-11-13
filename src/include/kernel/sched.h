#ifndef _CPU_SCHEDULER_H
#define _CPU_SCHEDULER_H

#include <asm/ptrace.h>
#include <asm/process.h>
#include <kernel/pid.h>
#include <kernel/signal.h>
#include <def/config.h>
#include <kernel/init.h>
#include <lib/list.h>
#include <fs/vfs.h>

#define TASK_EXIT_CODE_SIGNAL_MASK 0x007F
#define TASK_EXIT_CODE_CORE_MASK   0x0080
#define TASK_EXIT_CODE_STATUS_MASK 0xFF00
#define TASK_EXIT_CODE_STOPPED_MASK 0xFF00

#define TASK_STATUS_ENCODE_EXIT(exit_code) \
    (((uint16_t)((exit_code) & 0xFF)) << 8)

#define TASK_STATUS_ENCODE_SIGNAL(signal, core) \
    ((uint16_t)(((signal) & TASK_EXIT_CODE_SIGNAL_MASK) | ((core) ? TASK_EXIT_CODE_CORE_MASK : 0)))

#define TASK_STATUS_ENCODE_STOPPED(signal) \
    (((uint16_t)((signal) & 0xFF)) << 8)


struct mm_struct;
struct wait_queue_entry;
struct file_table;
struct file;

typedef enum {
	TASK_NEW,
	TASK_READY,
	TASK_RUNNING,
	TASK_ZOMBIE,
	TASK_INTERRUPTIBLE,
	TASK_BLOCKED,
} task_state_t;

struct task {
	pid_t pid;
	struct registers regs;

	char name[PROC_NAME_MAX];
	struct file_table *files;
	void* kstack;
	
	struct list_head tasks;
	struct list_head queue;

	struct mm_struct* mm;

	struct path cwd;

	task_state_t state;
	int priority;
	int exit_code;

	sigset_t sig_pending;
	sigset_t sig_blocked;
	struct sigaction sig_actions[NSIG];

	struct task* parent;
	pid_t pgid;
	pid_t sid;
	bool did_exec;

	struct list_head children;
	struct list_head sibling;
};

struct task* task_create(const char* name, int priority);

struct task* task_search(pid_t pid);
struct task *task_find_first_with_pgid(pid_t pgid);

void task_exit(struct task* task, int status);
void task_destroy(struct task* task);

void task_global_list_add(struct task* task);

int task_add_file(struct task* task, struct file* file);
int task_remove_file(struct task* task, int fd);

struct task* task_get_child(struct task* parent, pid_t pid);
struct task* task_find_zombie_child(struct task* parent);
void task_reparent_children(struct task* task, struct task* new_parent);

asmlinkage void task_sleep_uninterruptible(struct task* task);
asmlinkage void task_sleep_interruptible(struct task* task);
asmlinkage void task_wakeup(struct task* task);

int task_default_wakeup(struct wait_queue_entry* entry);

asmlinkage void schedule();

int __init scheduler_init();
void __init scheduler_start();
void scheduler_add(struct task* task);
void scheduler_remove(struct task* task);

static inline void sleep_uninterruptible(){
	task_sleep_uninterruptible(current);
	schedule();
}

static inline void sleep_interruptible(){
	task_sleep_interruptible(current);
	schedule();
}

#endif
