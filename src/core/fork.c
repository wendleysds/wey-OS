#include <kernel/sched.h>
#include <kernel/syscall.h>
#include <fs/fdtable.h>
#include <def/errno.h>
#include <mm/vma.h>
#include <fs/dcache.h>

static void copy_signals(struct task* to, const struct task *restrict from){
	for(int i = 0; i < NSIG; i++){
		to->sig_actions[i] = from->sig_actions[i];
	}

	to->sig_blocked = from->sig_blocked;
	to->sig_pending = from->sig_pending;
}

static struct task *copy_process() {
	struct task *cur = current;

	struct task *child = task_create(cur->name, cur->priority);
	if (!child){
		return ERR_PTR(-ENOMEM);
	}

	child->files = file_table_clone(cur->files);
	if(!child->files){
		task_destroy(child);
		return ERR_PTR(-ENOMEM);
	}

	child->pid = pid_alloc();
	if(child->pid < 0){
		task_destroy(child);
		return ERR_PTR(-ENOENT);
	}

	child->kstack = kmalloc(PROC_KERNEL_STACK_SIZE);
	if (!child->kstack) {
		task_destroy(child);
		return ERR_PTR(-ENOMEM);
	}

	copy_thread(cur, child, NULL, NULL);

	struct mm_struct *c_mm = mm_dup(cur->mm);
	if (!c_mm) {
		task_destroy(child);
		return ERR_PTR(-ENOMEM);
	}

	copy_signals(child, cur);
	child->sig_pending = 0;

	child->mm = c_mm;
	child->parent = cur;
	list_add(&child->sibling, &cur->children);

	child->cwd.dentry = dentry_get(cur->cwd.dentry);
	child->cwd.mount = cur->cwd.mount;

	task_global_list_add(child);
	return child;
}

pid_t kernel_thread(int (*fn)(void*), const char* name, void* args){
	struct task* task = task_create(name, 0);
	if(!task){
		return -ENOMEM;
	}

	task->kstack = kmalloc(PROC_KERNEL_STACK_SIZE);
	if(!task->kstack){
		task_destroy(task);
		return -ENOMEM;
	}

	task->pid = pid_alloc();
	if(task->pid < 0){
		task_destroy(task);
		return -ENOENT;
	}

	copy_thread(0x0, task, fn, args);
	task_global_list_add(task);

	task->state = TASK_READY;
	scheduler_add(task);
	return task->pid;
}

SYSCALL_DEFINE0(fork){
	struct task* child = copy_process();
	if(IS_ERR_VALUE(child)){
		return PTR_ERR(child);
	}

	child->state = TASK_READY;
	scheduler_add(child);
	return child->pid;
}
