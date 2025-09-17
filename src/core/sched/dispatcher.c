#include <core/sched.h>
#include <core/sched/task.h>
#include <core/rings.h>
#include <def/status.h>
#include <def/compile.h>
#include <memory/paging.h>

static struct Task* _currentTask = 0x0;

extern __no_return void pcb_return(struct Registers* regs);
extern void sheduler_enqueue_auto(struct Task* task);

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

int pcb_save_current_from_frame(struct InterruptFrame* frame){
    if(!frame){
        return NULL_PTR;
    }

    if(!_currentTask){
        return NO_TASKS;
    }

    _currentTask->regs.eax = frame->eax;
    _currentTask->regs.ebx = frame->ebx;
    _currentTask->regs.ecx = frame->ecx;
    _currentTask->regs.edx = frame->edx;

    _currentTask->regs.ss = frame->ss;
    _currentTask->regs.cs = frame->cs;

    _currentTask->regs.ebp = frame->ebp;
    _currentTask->regs.esp = frame->esp;
    _currentTask->regs.eip = frame->eip;

    _currentTask->regs.edi = frame->edi;
    _currentTask->regs.esi = frame->esi;

    _currentTask->regs.eflags = frame->eflags;

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
    mmu_page_switch(task->process->mm->pageDirectory);

    return SUCCESS;
}

struct Task* pcb_current(){
    return _currentTask;
}
