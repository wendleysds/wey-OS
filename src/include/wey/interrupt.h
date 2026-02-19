#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <asm/idt.h>
#include <asm/ptrace.h>

struct registers;

typedef void (*interrupt_handler_t)(struct registers*);

int interrupt_init();

void interrupts_enable();
void interrupts_disable();
void interrupt_register(int interrupt, interrupt_handler_t handler);

void interrupt_mask(uint8_t interrupt);
void interrupt_unmask(uint8_t interrupt);
void interrupt_eoi(uint8_t interrupt);

#endif
