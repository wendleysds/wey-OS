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

extern int __must_check dispatcher_load(struct Task* task);

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

	mmu_map_pages(
		(void*)PROC_KERNEL_STACK_VIRTUAL_BUTTOM, 
		mmu_translate(_idleTask.kernelStack), 
		PROC_KERNEL_STACK_SIZE, 
		(FPAGING_P | FPAGING_RW)
	);

	_idleTask.regs.eip = (uintptr_t)_idle_task_entry;
	_idleTask.regs.esp = (uintptr_t)_idleTask.kernelStack + PROC_KERNEL_STACK_SIZE;
	_idleTask.regs.ebp = _idleTask.regs.esp;
	_idleTask.regs.cs = KERNEL_CODE_SELECTOR;
}

static void _schedule_iqr_PIT_handler(struct InterruptFrame* frame){
	if(pcb_save_current_from_frame(frame) == NULL_PTR){
		panic("pcb_save_current_from_frame(*frame): Invalid frame pointer!");
	}

	pic_send_eoi(PIC_TIMER);
	schedule();
}

void schedule(){
	struct Task* next = scheduler_pick_next();
	if(!next){
		// if has a task running, continue with it
		if(pcb_current()){
			return;
		}

		next = &_idleTask;
	}

	next->state = TASK_RUNNING;
	if(dispatcher_load(next) != SUCCESS){
		panic("dispatcher_load(): Invalid task!");
	}
}

void schedule_next(struct Task* current){
	// save regs
	// schedule
}

void scheduler_start(){
	if(scheduling){
		return;
	}

	idt_register_callback(PIC_TIMER, _schedule_iqr_PIT_handler);
	ticks = 0;
	scheduling = 1;
}

void scheduler_init(){
	extern void pcb_set(struct Task* t);
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

struct Task* scheduler_pick_next(){
	return task_dequeue(&_readyQueue);
}
