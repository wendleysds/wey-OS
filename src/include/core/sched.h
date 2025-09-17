#ifndef _CPU_SCHEDULER_H
#define _CPU_SCHEDULER_H

#include <core/sched/task.h>
#include <arch/i386/idt.h>
#include <def/compile.h>
#include <stdint.h>

extern uint8_t scheduling;

void schedule();
void schedule_next(struct Task* current);

void scheduler_init();
void scheduler_start();
void scheduler_add_task(struct Task* task);
void scheduler_remove_task(struct Task* task);

struct Task* scheduler_pick_next();

// Process Control Block
int __must_check pcb_save_current_from_frame(struct InterruptFrame* frame);
int __must_check pcb_save_current_context(struct Registers* regs);

struct Task* pcb_current();
int pcb_switch(struct Task* task);

#endif
