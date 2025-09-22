#include <core/sched.h>
#include <core/sched/task.h>
#include <core/kernel.h>
#include <memory/kheap.h>
#include <lib/mem.h>
#include <def/status.h>
#include <stdint.h>
#include <drivers/terminal.h>
#include <io/ports.h>
#include <arch/i386/pic.h>

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
	return task_dequeue(&_readyQueue);
}			

static void _switch_to(struct Task* prev, struct Task* to){
	if(prev == to){
		return;
	}

	if(!to){
		if(prev && prev->state != TASK_WAITING){
			return;
		}

		to = &_idleTask;
	}

	to->state = TASK_RUNNING;
	if(pcb_load(to) != SUCCESS){
		panic("pcb_load(): Invalid task!");
	}
}

static void _schedule_iqr_PIT_handler(struct InterruptFrame* frame){
	struct Task* prev = pcb_current();
	if(pcb_save_from_frame(prev, frame) == NULL_PTR){
		panic("pcb_save_current_from_frame(*frame): Invalid frame pointer!");
	}

	pic_send_eoi(PIC_TIMER);
	struct Task* next = scheduler_pick_next();

	_switch_to(prev, next);
}

void schedule(){
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

	pcb_set(0x0);
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

void sheduler_enqueue_auto(struct Task* task){
	switch (task->state) {
		case TASK_RUNNING:
		case TASK_READY:
			task->state = TASK_READY;
			task_enqueue(&_readyQueue, task);
			break;
		case TASK_FINISHED:
			task_enqueue(&_terminateQueue, task);
			break;

		default:
			panic("sheduler_enqueue_auto(): Unknown task state!");
	}
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
