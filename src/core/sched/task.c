#include "def/status.h"
#include <lib/string.h>
#include <wey/sched.h>
#include <mm/kheap.h>
#include <def/err.h>

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

int task_destroy(struct task* task){
	return SUCCESS;
}