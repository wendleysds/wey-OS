#include "asm/process.h"
#include "asm/ptrace.h"
#include "def/compile.h"
#include "def/config.h"
#include "wey/pid.h"
#include <stdint.h>
#include <wey/panic.h>
#include <def/err.h>
#include <wey/sched.h>


/*
kernel_thread()
	start_thread_kernel()

fork()
	copy_process()
		copy task_struct
		copy mm
		copy fd table
		copy registers
		retorn child

	child->pid = pid_alloc()
	child->regs.ax = 0
	parent->regs.ax = child->pid

exec()
	load_elf()
	start_thread_user()
*/

asmlinkage __no_return void kernel_thread_trampoline(int (*fn)(void*), void* args){
	int ret = fn(args);
	// do exit
	panic("DEBUG: process \"%s\" exit with status %d.\n", current->name, ret);
	__builtin_unreachable();
}

int __init fork_init() {
	pid_init();
	return SUCCESS;
}

static struct task *copy_process() {
	struct task *cur = current;

	struct task *child = task_create(cur->name, cur->priority);
	if (!child)
		return NULL;

	void *new_kstack = kmalloc(PROC_KERNEL_STACK_SIZE);
	if (!new_kstack) {
		task_destroy(child);
		return NULL;
	}

	child->kstack = new_kstack;
	copy_thread(cur, child, 0x0, 0x0);

	struct mm_struct *c_mm = vma_dup(cur->mm);
	if (IS_ERR_OR_NULL(c_mm)) {
		kfree(new_kstack);
		task_destroy(child);
		return NULL;
	}

	child->mm = c_mm;

	for (int i = 0; i < PROC_FD_MAX; i++) {
		child->file_table[i] = cur->file_table[i];
		if (child->file_table[i]){
			file_get(child->file_table[i]);
		}
	}

	child->parent = cur;
	list_add(&child->sibling, &cur->children);

	return child;
}

pid_t kernel_thread(int (*fn)(void*), const char* name, void* args){
	struct task* task = task_create(name, 0);
	if(!task){
		return NO_MEMORY;
	}

	char* ksp = kmalloc(PROC_KERNEL_STACK_SIZE);
	if(!ksp){
		kfree(task);
		return NO_MEMORY;
	}

	task->pid = pid_alloc();
	if(task->pid == -1){
		kfree(ksp);
		kfree(task);
		return LIST_FULL;
	}

	task->kstack = ksp;
	copy_thread(0x0, task, fn, args);

	scheduler_add(task);
	return task->pid;
}
