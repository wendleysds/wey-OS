#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <def/config.h>
#include <lib/list.h>
#include <stdbool.h>
#include <stdint.h>

struct registers;

enum irq_id {
	IRQ_WR_TIMER,
	IRQ_ATA_PRIMARY,
	IRQ_ATA_SECONDARY,
	IRQ_KEYBOARD,
	IRQ_NOT_MAPPED,
	IRQ_MAX = TOTAL_INTERRUPTS
};

struct irq_chip {
	const char *name;
	int (*init)(int freq);
	void (*enable)(void);
	void (*disable)(void);
	void (*eoi)(int irq);
	void (*mask)(int irq);
	void (*unmask)(int irq);
};

struct irq_cpu_context {
	int cpu_id;
	struct registers* regs;
	bool from_user;

	bool exception;
	const char* exception_name;
};

struct irq_routing_info {
	enum irq_id irq_id;
	unsigned long hw_line;
};

struct irq_info {
	struct irq_cpu_context cpu;
	struct irq_routing_info route;

	bool needs_eoi;
	void* device;
};

typedef void (*interrupt_handler_t)(struct irq_info*);

struct irq_handler_node {
	interrupt_handler_t handler;
	void* device;
	struct irq_handler_node* next;
};

struct irq_desc {
	uint32_t hw_line;
	struct irq_chip* chip;

	bool masked;
	struct irq_handler_node* handlers;
};

int interrupt_init();
int generic_handle_irq(struct irq_info* info);

// For raw interrupts
int interrupt_register(int interrupt, interrupt_handler_t handler, void *dev);
int interrupt_unregister(int interrupt, interrupt_handler_t handler, void *dev);

int irq_register(enum irq_id irq, interrupt_handler_t handler, void *dev);
int irq_unregister(enum irq_id irq, interrupt_handler_t handler, void *dev);
void irq_mask(enum irq_id irq);
void irq_unmask(enum irq_id irq);

void interrupts_enable();
void interrupts_disable();

void interrupt_mask(int interrupt);
void interrupt_unmask(int interrupt);
void interrupt_eoi(int interrupt);

// chips
void irq_set_chip(int irq, struct irq_chip* chip);
struct irq_chip* irq_get_chip(int irq);

#endif
