#include <wey/sched.h>
#include <wey/panic.h>
#include <mm/kheap.h>
#include <lib/string.h>
#include <def/err.h>
#include <drivers/terminal.h>
#include <io/ports.h>
#include <arch/i386/pic.h>
#include <stdint.h>

#define PIC_TIMER 0x20

extern int __must_check pcb_load(struct Task* task);
extern void pcb_set(struct Task* t);

static struct TaskQueue _readyQueue;
static struct TaskQueue _terminateQueue;

static volatile uint64_t ticks = 0;
uint8_t scheduling = 0; // Started?

static struct Task _idleTask;

static void _idle_task_entry(){
	while (1) {
		__asm__ volatile ("hlt");
	}
}

static void init_task_idle(){
	memset(&_idleTask, 0x0, sizeof(_idleTask));

	_idleTask.tid = 0;
	_idleTask.state = TASK_READY;
	_idleTask.priority = 0;
	_idleTask.process = NULL;

	_idleTask.kernelStack = kmalloc(PROC_KERNEL_STACK_SIZE);
	_idleTask.userStack = NULL;

	_idleTask.regs.eip = (uintptr_t)_idle_task_entry;
	_idleTask.regs.esp = (uintptr_t)_idleTask.kernelStack + PROC_KERNEL_STACK_SIZE;
	_idleTask.regs.ebp = _idleTask.regs.esp;
	_idleTask.regs.cs = KERNEL_CODE_SELECTOR;
}

static struct Task* scheduler_pick_next(){
	struct Task* t = task_dequeue(&_readyQueue);
	if(!t) t = &_idleTask;
	return t;
}			

static void _switch_to(struct Task* prev, struct Task* to){
	if(prev == to){
		return;
	}

	if(to->tid == 0 && prev->state != TASK_WAITING){
		return;
	}

	if(prev && prev->tid != 0){
		if(prev->state == TASK_RUNNING || prev->state == TASK_READY){
			prev->state = TASK_READY;
			task_enqueue(&_readyQueue, prev);
		}else if(prev->state == TASK_FINISHED){
			task_enqueue(&_terminateQueue, prev);
		}else if(prev->state != TASK_WAITING){
			panic("_switch_to(): Unknown task state!");
		}
	}

	to->state = TASK_RUNNING;
	if(IS_STAT_ERR(pcb_load(to))){
		panic("pcb_load(): Invalid task!");
	}
}

static void _schedule_iqr_PIT_handler(struct InterruptFrame* frame){
	if(!scheduling){
		return;
	}

	struct Task* prev = pcb_current();

	uint32_t idle_task_esp = prev->regs.esp;
	if(IS_STAT_ERR(pcb_save_from_frame(prev, frame))){
		panic("pcb_save_current_from_frame(*frame): Invalid frame pointer!");
	}

	// tmp hack
	if(prev && prev->tid == 0){
		prev->regs.esp = idle_task_esp;
	}

	pic_send_eoi(PIC_TIMER);
	struct Task* next = scheduler_pick_next();

	_switch_to(prev, next);
}

void schedule(){
	if(!scheduling){
		return;
	}

	struct Task* prev = pcb_current();

	if(pcb_save_context(&prev->regs) == 0){
		return;
	}

	struct Task* next = scheduler_pick_next();

	_switch_to(prev, next);
}

void scheduler_start(){
	if(scheduling){
		return;
	}

	pcb_set(&_idleTask);
	idt_register_callback(PIC_TIMER, _schedule_iqr_PIT_handler);
	ticks = 0;
	scheduling = 1;
}

void scheduler_init(){
	pcb_set(NULL);

	init_task_idle();

	memset(&_readyQueue, 0x0, sizeof(struct TaskQueue));
	memset(&_terminateQueue, 0x0, sizeof(struct TaskQueue));

	scheduling = 0;
}

void scheduler_add_task(struct Task* task){
	task->state = TASK_READY;
	task_enqueue(&_readyQueue, task);
}

void scheduler_remove_task(struct Task* task){
	struct TaskQueue* queue;
	switch (task->state)
	{
	case TASK_READY:
		queue = &_readyQueue;
		break;
	case TASK_FINISHED:
		queue = &_terminateQueue;
		break;
	default:
		return;
	}

	task_queue_remove(queue, task);
}
