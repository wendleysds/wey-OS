#include <core/sched.h>
#include <core/sched/task.h>
#include <core/rings.h>
#include <def/status.h>
#include <memory/paging.h>

static struct Task* _currentTask = 0x0;

extern void pcb_return(struct Registers* regs);
extern void sheduler_enqueue_auto(struct Task* task);

static void pcb_save(struct Task* task, struct InterruptFrame* frame){
    task->regs.eax = frame->eax;
    task->regs.ebx = frame->ebx;
    task->regs.ecx = frame->ecx;
    task->regs.edx = frame->edx;

    task->regs.ss = frame->ss;
    task->regs.cs = frame->cs;

    task->regs.ebp = frame->ebp;
    task->regs.esp = frame->esp;
    task->regs.eip = frame->eip;

    task->regs.edi = frame->edi;
    task->regs.esi = frame->esi;

    task->regs.eflags = frame->eflags;
}

static inline int pcb_load(struct Task* task){
    if(task->tid == 0){
        _currentTask = task;
        pcb_return(&task->regs);
        return SUCCESS;
    }

    if(!task || !task->process){
        return INVALID_ARG; // Task null or dont have a process associated
    }

    if(_currentTask && _currentTask->tid != 0){
        sheduler_enqueue_auto(_currentTask);
    }
    
    _currentTask = task;

    pcb_switch(task);
    pcb_return(&task->regs);

    return SUCCESS;
}

int __must_check dispatcher_load(struct Task* task){
    return pcb_load(task);
}

void pcb_set(struct Task* t){
	_currentTask = t;
}

int pcb_save_current(struct InterruptFrame* frame){
    if(!frame){
        return NULL_PTR;
    }

    if(!_currentTask){
        return NO_TASKS;
    }

    pcb_save(_currentTask, frame);
    return SUCCESS;
}

int pcb_switch(struct Task* task){
    if(!task || !task->process){
        return INVALID_ARG; // Task null or dont have a process associated
    }

    if(!task->process->mm->pageDirectory){
        return NULL_PTR;
    }

    _currentTask = task;
    paging_switch(task->process->mm->pageDirectory);

    return SUCCESS;
}

int pcb_page_current(){
	if(!scheduling){
		return NOT_READY;
	}

    if(!_currentTask){
        return NULL_PTR;
    }

    user_registers();
    return pcb_switch(_currentTask);
}

struct Task* pcb_current(){
    return _currentTask;
}
