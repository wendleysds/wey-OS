#include <wey/syscall.h>
#include <wey/sched.h>
#include <wey/panic.h>
#include <mm/kheap.h>
#include <lib/assert.h>

struct task* init_task;

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

asmlinkage void task_sleep(struct task* task){
	task->state = TASK_BLOCKED;
}

asmlinkage void task_wakeup(struct task* task){
	if(task->state == TASK_BLOCKED){
		task->state = TASK_READY;
		scheduler_add(task);
	}
}

void task_reparent_children(struct task* task, struct task* new_parent){
	struct task* child;
	struct task* tmp;

	list_for_each_entry_safe(child, tmp, &task->children, sibling){
		child->parent = new_parent;

		list_remove(&child->sibling);
		list_add(&child->sibling, &new_parent->children);
	}
}

void task_exit(struct task* task, int status){
	if(unlikely(task->pid == 1)){
		panic("Attempting to exit init process");
	}

	task->exit_code = status;
	task->state = TASK_ZOMBIE;

	task_reparent_children(task, init_task);

	task_wakeup(task->parent);
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

	if(task->mm){
		task_destroy_mm(task);
		task->mm = NULL;
	}

	task_close_files(task);

	list_remove(&task->tasks);
	list_remove(&task->sibling);
	pid_free(task->pid);
	kfree(task);
}

void asmlinkage task_handle_prev_status(struct task* prev){
	if(unlikely(prev->pid == 0)){
		return;
	}

	switch (prev->state) {
		case TASK_ZOMBIE:
			if(prev->pwd){
				kfree(prev->pwd);
				prev->pwd = NULL;
			}

			if(prev->kstack){
				kfree(prev->kstack);
				prev->kstack = NULL;
			}

			if(prev->mm){
				task_destroy_mm(prev);
				prev->mm = NULL;
			}
			break;
		case TASK_RUNNING:
			prev->state = TASK_READY;
			scheduler_add(prev); 
			return;
		default: return;
	}
}

struct task* task_get_child(struct task* parent, pid_t pid){
	struct task* child;
	list_for_each_entry(child, &parent->children, sibling){
		if(child->pid == pid){
			return child;
		}
	}

	return NULL;
}

struct task* task_find_zombie_child(struct task* parent){
	struct task* child = NULL;

	list_for_each_entry(child, &parent->children, sibling){
		if(child->state == TASK_ZOMBIE){
			return child;
		}
	}

	return NULL;
}

SYSCALL_DEFINE1(exit, int, status){
	task_exit(current, status);
	schedule();
	unreachable();
}