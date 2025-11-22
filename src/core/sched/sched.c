#include <wey/sched.h>
#include <wey/panic.h>
#include <asm/context.h>
#include <def/compile.h>
#include <lib/string.h>

static LIST_HEAD(_readyQueue);
static LIST_HEAD(_terminateQueue);

static struct task* _next(){
	struct list_head* pos;
	struct task* next_task = 0x0;

	if(_readyQueue.next == &_readyQueue){
		return 0x0;
	}

	next_task = list_entry(_readyQueue.next, struct task, tasks);
	return next_task;
}

asmlinkage void schedule(){
	struct task* next_task = _next();
	struct task* prev_task = current;

	if(next_task == 0x0){
		if(prev_task->state != TASK_FINISHED){
			return;
		}

		// next_task = idle_task
		panic("no tasks!");
	}

	context_switch(prev_task, next_task);
}

static void scheduler_tick(){
}

int __init scheduler_init(){
	INIT_LIST_HEAD(&_readyQueue);
	INIT_LIST_HEAD(&_terminateQueue);
}

void scheduler_add(struct task* task){
	list_add_tail(&task->tasks, &_readyQueue);
}

void scheduler_remove(struct task* task){
	list_remove(&task->tasks);
}