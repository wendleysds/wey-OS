#ifndef _CPU_SCHEDULER_H
#define _CPU_SCHEDULER_H

#include <wey/process.h>
#include <arch/i386/idt.h>
#include <def/compile.h>
#include <stdint.h>

extern uint8_t scheduling;

asmlinkage void schedule();

void scheduler_init();
void scheduler_start();
void scheduler_add_task(struct Task* task);
void scheduler_remove_task(struct Task* task);

// Process Control Block
int __must_check pcb_save_from_frame(struct Task* task, struct InterruptFrame* frame);
int __must_check pcb_save_context(struct Registers* regs);

struct Task* pcb_current();
int __must_check pcb_switch(struct Task* task);

#endif
