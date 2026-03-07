#include "asm/paging.h"
#include "def/status.h"
#include "lib/list.h"
#include "wey/vfs.h"
#include "wey/vma.h"
#include <lib/string.h>
#include <wey/sched.h>
#include <wey/pid.h>
#include <wey/panic.h>
#include <mm/kheap.h>
#include <def/err.h>
#include <def/compile.h>

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

	if(task->mm->pgd){
		pgd_free(task->mm->pgd);
	}

	vma_destroy(task->mm);

	kfree(task->mm);
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

void __no_return task_exit(struct task* task, int status){
	task->exit_code = status;

	if(task->pwd){
		kfree(task->pwd);
		task->pwd = NULL;
	}

	// TODO: create init task
	//reparent_children(task);

	task_close_files(task);
	task_destroy_mm(task);

	task->state = TASK_ZOMBIE;

	//TODO: wakeup parent if waiting PID

	schedule();
	panic("Finished task returned!");

	__builtin_unreachable();
}

int task_destroy(struct task* task){
	if(task->state != TASK_ZOMBIE){
		return INVALID_STATE;
	}

	list_remove(&task->tasks);
	pid_free(task->pid);
	kfree(task);
	return SUCCESS;
}
