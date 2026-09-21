#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <def/config.h>
#include <lib/list.h>
#include <sync/atomic.h>
#include <stdbool.h>
#include <stdint.h>

struct registers;
struct irq_desc;

enum irq_trigger_type {
	IRQ_TYPE_EDGE,
	IRQ_TYPE_LEVEL,
};

enum irq_polarity {
	IRQ_POLARITY_HIGH,
	IRQ_POLARITY_LOW,
};

struct irq_chip {
	const char *name;

	void (*mask)(struct irq_desc *);
	void (*unmask)(struct irq_desc *);

	int (*set_type)(struct irq_desc *, enum irq_trigger_type type);
	int (*set_affinity)(struct irq_desc *, unsigned int cpu);
};

struct irq_controller {
	const char *name;
	void (*eoi)(struct irq_desc *);
};

struct irq_domain {
	const char *name;

	int (*map)(
		struct irq_domain *,
		unsigned int hwirq,
		unsigned int *irq
	);

	void (*unmap)(
		struct irq_domain *,
		unsigned int hwirq
	);

	const struct irq_chip *chip;
	const struct irq_controller *controller;
};

struct irq_desc {
	unsigned int irq;
	unsigned int hwirq;

	enum irq_trigger_type type;
	enum irq_polarity polarity;

	const struct irq_chip *chip;
	const struct irq_controller *controller;

	struct irq_domain *domain;

	void *chip_data;

	bool masked;

	struct irq_handler_node *handlers;

	atomic_t refcount;
};

struct irq_cpu_context {
	int cpu_id;
	struct registers* regs;
	bool from_user;

	bool exception;
	const char* exception_name;
};

struct irq_info {
	struct irq_cpu_context cpu;
	int hwirq;

	bool needs_eoi;
	void* device;
};

typedef void (*interrupt_handler_t)(struct irq_info*);

struct irq_handler_node {
	interrupt_handler_t handler;
	void* device;
	struct irq_handler_node* next;
};

int interrupt_init();
int generic_handle_irq(struct irq_info* info);

// For raw interrupts
int interrupt_register(int interrupt, interrupt_handler_t handler, void *dev);
int interrupt_unregister(int interrupt, interrupt_handler_t handler, void *dev);

void interrupts_enable();
void interrupts_disable();

void interrupt_mask(int interrupt);
void interrupt_unmask(int interrupt);
void interrupt_eoi(int interrupt);

void interrupt_set_chip(int interrupt, struct irq_chip* chip);
void interrupt_set_controller(int interrupt, struct irq_controller* controller);

const struct irq_chip* interrupt_get_chip(int interrupt);
const struct irq_controller* interrupt_get_controller(int interrupt);

#endif
