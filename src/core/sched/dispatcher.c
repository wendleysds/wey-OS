#include <wey/sched.h>
#include <wey/process.h>
#include <def/status.h>
#include <def/compile.h>
#include <mm/paging.h>

static struct Task* _currentTask = 0x0;

extern __no_return void pcb_return(struct Registers* regs);

int __must_check pcb_load(struct Task* task){
	if(task->tid == 0){
		_currentTask = task;
		goto ret;
	}

	if(!task || !task->process){
		return INVALID_ARG;
	}

	int res;
	if((res = pcb_switch(task)) != SUCCESS){
		return res;
	}

ret:
	pcb_return(&task->regs);
	__builtin_unreachable();
}

void pcb_set(struct Task* t){
	_currentTask = t;
}

int pcb_save_from_frame(struct Task* task, struct InterruptFrame* frame){
    if(!frame || !task){
        return NULL_PTR;
    }

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
    return mmu_page_switch(task->process->mm->pageDirectory);
}

struct Task* pcb_current(){
    return _currentTask;
}
