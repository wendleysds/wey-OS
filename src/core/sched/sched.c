#include <kernel/clock.h>
#include <kernel/sched.h>
#include <kernel/printk.h>
#include <sync/spinlock.h>
#include <def/compile.h>
#include <def/errno.h>
#include <lib/string.h>
#include <mm/kheap.h>

static LIST_HEAD(_readyQueue);
static LIST_HEAD(_terminateQueue);

static volatile uint8_t scheduling = 0;
static spinlock_t scheduler_spinlock;
volatile int need_resched = 0;

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

// Request resched on every tick
static void scheduler_tick(void* unused){
	need_resched = 1;
}

asmlinkage void schedule(){
	if(unlikely(!scheduling)){
		return;
	}

	struct task* next_task = _next();
	struct task* prev_task = current;

	if(unlikely(next_task == NULL)){
		if(prev_task->state == TASK_BLOCKED){
			next_task = &idle_task;
		} else{
			next_task = prev_task;
		}
	}

	printk("Switching to \"[%d:%s]\"\n", next_task->pid, next_task->name);

	context_switch(prev_task, next_task);
}

static int __init create_idle_task(){
	memset(&idle_task, 0x0, sizeof(struct task));
	void* ksp = kzalloc(PROC_KERNEL_STACK_SIZE);
	if(!ksp){
		return -ENOMEM;
	}

	memcpy(idle_task.name, "idle task", sizeof(idle_task.name));
	idle_task.kstack = ksp;
	copy_thread(0x0, &idle_task, idle_task_routine, 0x0);
	idle_task.pid = 0;

	current = &idle_task;

	return SUCCESS;
}

int __init scheduler_init(){
	spinlock_init(&scheduler_spinlock);
	INIT_LIST_HEAD(&_readyQueue);
	INIT_LIST_HEAD(&_terminateQueue);
	scheduling = 0;

	if(clockevent_register_listener(scheduler_tick, 0x0) != SUCCESS){
		return -ENOMEM;
	}

	return create_idle_task();
}

void __init scheduler_start(){
	scheduling = 1;
}

void scheduler_add(struct task* task){
	spin_lock(&scheduler_spinlock);
	list_add_tail(&task->queue, &_readyQueue);
	spin_unlock(&scheduler_spinlock);
}

void scheduler_remove(struct task* task){
	spin_lock(&scheduler_spinlock);
	list_remove(&task->queue);
	spin_unlock(&scheduler_spinlock);
}
