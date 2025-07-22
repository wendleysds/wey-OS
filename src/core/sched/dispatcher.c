#include <core/sched.h>
#include <core/sched/task.h>
#include <def/status.h>

/*
static struct Task* current = 0x0;
*/

extern void pcb_return(struct Registers* regs);

void pcb_save(struct Task* task, struct InterruptFrame* frame){
    // TODO: Implement
}

void pcb_switch(struct Task* task){
    // TODO: Implement
}

int pcb_load(struct Task* task){
    if(!task || !task->process){
        return INVALID_ARG;
    }

    return SUCCESS;
}

