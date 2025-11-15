#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <asm/idt.h>
#include <asm/ptrace.h>

struct registers;

typedef void (*irq_handler_t)(struct registers*);

void interrupts_enable();
void interrupts_disable();
void interrupt_register(int interrupt, irq_handler_t handler);

#endif
