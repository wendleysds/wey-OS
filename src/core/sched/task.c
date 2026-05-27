#include <wey/syscall.h>
#include <wey/sched.h>
#include <wey/panic.h>
#include <mm/kheap.h>
#include <lib/assert.h>

struct task* task_create(const char* name, int priority){ 
	struct task* new_task = kmalloc(sizeof(struct task)); 
	if(likely(new_task)){ 
		memset(new_task, 0, sizeof(struct task)); 
		strncpy(new_task->name, name, PROC_NAME_MAX); 
		new_task->priority = priority; 
		new_task->state = TASK_NEW; 
		INIT_LIST_HEAD(&new_task->tasks); 
		INIT_LIST_HEAD(&new_task->queue); 
		INIT_LIST_HEAD(&new_task->children); 
		INIT_LIST_HEAD(&new_task->sibling); 
	} 

	return new_task; 
}

static void task_destroy_mm(struct task* task){
	if(!task->mm){
		return;
	}

	vma_put(task->mm);
	task->mm = NULL;
}

static void task_close_files(struct task* task){
	for(int i = 0; i < PROC_FD_MAX; i++){
		if(task->file_table[i] != NULL){
			vfs_close(task->file_table[i]);
			task->file_table[i] = NULL;
		}
	}
}

/*void reparent_children(struct task* task){
	struct task* child;
	struct task* tmp;

	list_for_each_entry_safe(child, tmp, &task->children, sibling){
		child->parent = init_task;

		list_remove(&child->sibling);
		list_add(&child->sibling, &init_task->children);
	}
}*/

void task_exit(struct task* task, int status){
	task->exit_code = status;
	task->state = TASK_ZOMBIE;

	// wakeup_parent(task);
}

void task_destroy(struct task* task){
	if(task->state != TASK_ZOMBIE){
		panic("Attempting to destroy a non-zombie task (pid: %d, name: %s)", task->pid, task->name);
	}

	if(task->pwd){
		kfree(task->pwd);
		task->pwd = NULL;
	}

	if(task->kstack){
		kfree(task->kstack);
		task->kstack = NULL;
	}

	task_destroy_mm(task);
	task_close_files(task);

	/*reparent_children(task);*/	

	list_remove(&task->tasks);
	pid_free(task->pid);
	kfree(task);
}

void task_handle_state(struct task* task){
	switch (task->state) {
		case TASK_RUNNING:
			if(likely(task->pid	!=0)){
				task->state = TASK_READY;
				scheduler_add(task); 
			}
			return;
		default: return;
	}
}

SYSCALL_DEFINE1(exit, int, status){
	task_exit(current, status);
	schedule();
	unreachable();
}