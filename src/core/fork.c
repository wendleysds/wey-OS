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

void __init fork_init() {}

struct task *copy_process() {
	struct task *cur = current;

	struct task *child = task_create(cur->name, cur->priority);
	if (!child)
		return NULL;

	void *new_kstack = kmalloc(PROC_KERNEL_STACK_SIZE);
	if (!new_kstack) {
		task_destroy(child);
		return NULL;
	}

	memcpy(new_kstack, cur->kstack, PROC_KERNEL_STACK_SIZE);
	child->kstack = new_kstack;

	copy_thread(cur, child);

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

pid_t kernel_clone();
pid_t kernel_thread();
pid_t user_mode_thread();