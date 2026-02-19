#include "wey/printk.h"
#include <wey/sched.h>
#include <wey/panic.h>
#include <asm/context.h>
#include <def/compile.h>
#include <def/status.h>
#include <lib/string.h>
#include <wey/interrupt.h>
#include <wey/timer.h>

static LIST_HEAD(_readyQueue);
static LIST_HEAD(_terminateQueue);

static struct task* _next(){
	struct task* next_task = 0x0;

	if(_readyQueue.next == &_readyQueue){
		return 0x0;
	}

	next_task = list_entry(_readyQueue.next, struct task, tasks);
	list_remove(&next_task->tasks);

	return next_task;
}

asmlinkage void schedule(){
	struct task* next_task = _next();
	struct task* prev_task = current;

	if(next_task == 0x0){
		if(prev_task->state == TASK_RUNNING){
			return;
		}

		// next_task = idle_task
		panic("no tasks!");
	}

	if(likely(prev_task))
		scheduler_add(prev_task);

	context_switch(prev_task, next_task);
}

static void scheduler_tick(struct registers* regs){
	schedule();
}

int __init scheduler_init(){
	INIT_LIST_HEAD(&_readyQueue);
	INIT_LIST_HEAD(&_terminateQueue);

	timer_register_callback(scheduler_tick);

	return SUCCESS;
}

void scheduler_add(struct task* task){
	task->state = TASK_READY;
	list_add_tail(&task->tasks, &_readyQueue);
}

void scheduler_remove(struct task* task){
	list_remove(&task->tasks);
}