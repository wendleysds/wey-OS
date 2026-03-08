#include "asm/process.h"
#include "def/config.h"
#include "wey/spinlock.h"
#include <wey/printk.h>
#include <wey/sched.h>
#include <wey/panic.h>
#include <def/compile.h>
#include <def/status.h>
#include <lib/string.h>
#include <wey/interrupt.h>

static LIST_HEAD(_readyQueue);
static LIST_HEAD(_terminateQueue);

static volatile uint8_t scheduling = 0;
static spinlock_t scheduler_spinlock;

static struct task idle_task;

static int idle_task_routine(void* args){
	while(1){
		cpu_relax();
	}

	__builtin_unreachable();
}

static struct task* _next(){
	struct task* next_task = 0x0;

	spin_lock(&scheduler_spinlock);

	if(likely(!list_empty(&_readyQueue))){
		next_task = list_entry(_readyQueue.next, struct task, queue);
		list_remove(&next_task->queue);
	}

	spin_unlock(&scheduler_spinlock);

	return next_task;
}

asmlinkage void schedule(){
	if(unlikely(!scheduling)){
		return;
	}

	struct task* next_task = _next();
	struct task* prev_task = current;

	if(next_task == 0x0){
		if(prev_task->state == TASK_RUNNING){
			printk("resuming: '%s'\n", prev_task->name);
			return;
		}

		next_task = &idle_task;
		//panic("no tasks!");
	}

	printk("Switching to '%s'\n", next_task->name);

	if(likely(prev_task)){
		if(prev_task->state != TASK_SLEEPING && 
			prev_task->state != TASK_ZOMBIE){
			scheduler_add(prev_task);
		}
	}

	context_switch(prev_task, next_task);
}

static int __init create_idle_task(){
	memset(&idle_task, 0x0, sizeof(struct task));
	void* ksp = kzalloc(PROC_KERNEL_STACK_SIZE);
	if(!ksp){
		return NO_MEMORY;
	}

	memcpy(idle_task.name, "idle task", sizeof(idle_task.name));
	idle_task.kstack = ksp;
	copy_thread(0x0, &idle_task, idle_task_routine, 0x0);
	idle_task.pid = 0;

	return SUCCESS;
}

int __init scheduler_init(){
	spinlock_init(&scheduler_spinlock);
	INIT_LIST_HEAD(&_readyQueue);
	INIT_LIST_HEAD(&_terminateQueue);
	scheduling = 0;

	return create_idle_task();
}

void __init scheduler_start(){
	scheduling = 1;
}

void scheduler_add(struct task* task){
	spin_lock(&scheduler_spinlock);
	task->state = TASK_READY;
	list_add_tail(&task->queue, &_readyQueue);
	spin_unlock(&scheduler_spinlock);
}

void scheduler_remove(struct task* task){
	spin_lock(&scheduler_spinlock);
	list_remove(&task->queue);
	spin_unlock(&scheduler_spinlock);
}
