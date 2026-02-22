#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <def/config.h>

#include <asm/idt.h>
#include <asm/ptrace.h>
#include <stdint.h>

typedef enum {
	IRQ_WR_TIMER,
	IRQ_ATA_PRIMARY,
	IRQ_ATA_SECONDARY,
	IRQ_KEYBOARD,
	IRQ_NOT_MAPPED,
	IRQ_MAX = TOTAL_INTERRUPTS
} irq_id_t;

struct irq_cpu_context {
	struct registers* regs;
	int cpu_id;
	char from_user;
};

struct irq_routing_info {
	irq_id_t irq_id;
	unsigned long hw_line;
};

struct irq_info {
	struct irq_cpu_context cpu;
	struct irq_routing_info route;

	char needs_eoi;
	void* device;
};

typedef void (*interrupt_handler_t)(struct irq_info*);

struct irq_handler_node {
	interrupt_handler_t handler;
	void* device;
	struct irq_handler_node* next;
};

struct irq_desc {
	struct irq_handler_node* handlers;
	char masked;
	uint32_t hw_line;
};

int interrupt_init();

// For raw interrupts
int interrupt_register(int interrupt, interrupt_handler_t handler, void *dev);
int interrupt_unregister(int interrupt, interrupt_handler_t handler, void *dev);

int irq_register(irq_id_t irq, interrupt_handler_t handler, void *dev);
int irq_unregister(irq_id_t irq, interrupt_handler_t handler, void *dev);

void interrupts_enable();
void interrupts_disable();

void interrupt_mask(uint8_t interrupt);
void interrupt_unmask(uint8_t interrupt);
void interrupt_eoi(uint8_t interrupt);

#endif
