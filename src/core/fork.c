#include <wey/sched.h>
#include <wey/printk.h>
#include <wey/syscall.h>
#include <def/err.h>

static struct task *copy_process() {
	struct task *cur = current;

	struct task *child = task_create(cur->name, cur->priority);
	if (!child){
		return ERR_PTR(NO_MEMORY);
	}

	pid_t child_pid = pid_alloc();
	if(child_pid < 0){
		kfree(child);
		return ERR_PTR(LIST_FULL);
	}

	void *new_kstack = kmalloc(PROC_KERNEL_STACK_SIZE);
	if (!new_kstack) {
		kfree(child);
		pid_free(child_pid);
		return ERR_PTR(NO_MEMORY);
	}

	child->kstack = new_kstack;
	copy_thread(cur, child, 0x0, 0x0);

	struct mm_struct *c_mm = vma_dup(cur->mm);
	if (!c_mm) {
		kfree(new_kstack);
		kfree(child);
		pid_free(child_pid);
		return ERR_PTR(NO_MEMORY);
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

	child->pid = child_pid;
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

	task->state = TASK_READY;
	scheduler_add(task);
	return task->pid;
}

SYSCALL_DEFINE0(fork){
	struct task* child = copy_process();
	if(IS_ERR_VALUE(child)){
		return PTR_ERR(child);
	}

	scheduler_add(child);
	return child->pid;
}
