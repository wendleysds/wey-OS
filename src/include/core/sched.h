#ifndef _CPU_SCHEDULER_H
#define _CPU_SCHEDULER_H

#include <core/sched/task.h>
#include <arch/i386/idt.h>
#include <stdint.h>

void schedule();
void scheduler_init();
void schedule_add_task(struct Task* task);
struct Task* scheduler_pick_next();

void pcb_switch(struct Task* task);
void pcb_save(struct Task* task, struct InterruptFrame* frame);

#endif
