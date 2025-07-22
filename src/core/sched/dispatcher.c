#include <core/sched.h>
#include <core/sched/task.h>
#include <core/rings.h>
#include <def/status.h>
#include <memory/paging.h>

static struct Task* current = 0x0;

extern void pcb_return(struct Registers* regs);

static void pcb_save(struct Task* task, struct InterruptFrame* frame){
    task->regs.eax = frame->eax;
    task->regs.ebx = frame->ebx;
    task->regs.ecx = frame->ecx;
    task->regs.edx = frame->edx;

    task->regs.ss = frame->ss;
    task->regs.cs = frame->cs;

    task->regs.ebp = frame->ebp;
    task->regs.esp = frame->esp;
    task->regs.eip = frame->ip;

    task->regs.edi = frame->edi;
    task->regs.esi = frame->esi;

    task->regs.eflags = frame->eflags;
}

static int pcb_load(struct Task* task){
    if(!task || !task->process){
        return INVALID_ARG; // Task null or dont have a process associated
    }

    pcb_switch(task);
    pcb_return(&task->regs);

    return SUCCESS;
}

int pcb_save_current(struct InterruptFrame* frame){
    if(!frame){
        return INVALID_ARG;
    }

    if(!current){
        return NO_TASKS;
    }

    pcb_save(current, frame);
    return SUCCESS;
}

int pcb_switch(struct Task* task){
    if(!task || !task->process){
        return INVALID_ARG; // Task null or dont have a process associated
    }

    if(!task->process->pageDirectory){
        return NULL_PTR;
    }

    current = task;
    paging_switch(task->process->pageDirectory);

    return SUCCESS;
}

int pcb_page_current(){
    user_registers();
    return pcb_switch(current);
}
