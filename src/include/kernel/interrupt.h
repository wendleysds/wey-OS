#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <def/config.h>
#include <lib/list.h>
#include <sync/atomic.h>
#include <stdbool.h>
#include <stdint.h>

struct registers;
struct irq_desc;
struct irq_domain;

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
		struct irq_domain *d,
		unsigned int hwirq,
		unsigned int *irq
	);

	void (*unmap)(
		struct irq_domain *d,
		unsigned int hwirq
	);

	const struct irq_chip *chip;
	const struct irq_controller *controller;

	struct list_head list;
	void *host_data;
	unsigned int hwirq_base;
	unsigned int irq_base;
	unsigned int nr_irqs;
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

int interrupt_init(void);
int generic_handle_irq(struct irq_info* info);

// Interrupt registration
int interrupt_register(int interrupt, interrupt_handler_t handler, void *dev);
int interrupt_unregister(int interrupt, interrupt_handler_t handler, void *dev);

void interrupts_enable(void);
void interrupts_disable(void);

void interrupt_mask(int interrupt);
void interrupt_unmask(int interrupt);
void interrupt_eoi(int interrupt);

// IRQ descriptor management
int irqdesc_init(void (*callback)(int hwirq, struct irq_desc* desc));
struct irq_desc* irq_desc_get(int interrupt);
struct irq_desc* irq_desc_get_by_hwirq(int hwirq);
struct irq_desc* irq_to_desc(int irq);

// Chip management & switching
void interrupt_set_chip(int interrupt, const struct irq_chip* chip);
void interrupt_set_controller(int interrupt, const struct irq_controller* controller);
void irq_set_chip(int irq, const struct irq_chip *chip);
void irq_set_controller(int irq, const struct irq_controller *controller);
void irq_set_chip_and_controller(int irq, const struct irq_chip *chip, const struct irq_controller *controller);
void irq_set_chip_data(int irq, void *data);
void *irq_get_chip_data(int irq);
int irq_switch_chip(int irq, const struct irq_chip *new_chip, const struct irq_controller *new_controller);
int irq_switch_all_chips(const struct irq_chip *new_chip, const struct irq_controller *new_controller);

const struct irq_chip* interrupt_get_chip(int interrupt);
const struct irq_controller* interrupt_get_controller(int interrupt);
const struct irq_chip* irq_get_chip(int irq);
const struct irq_controller* irq_get_controller(int irq);

int irq_set_trigger_type(int irq, enum irq_trigger_type type);
int irq_set_affinity(int irq, unsigned int cpu);

// IRQ Domain API
int irq_domain_register(struct irq_domain *domain);
void irq_domain_unregister(struct irq_domain *domain);
struct irq_domain *irq_domain_get_default(void);
void irq_domain_set_default(struct irq_domain *domain);
struct irq_domain *irq_domain_find_by_name(const char *name);

struct irq_domain *irq_domain_create_legacy(
	const char *name,
	unsigned int nr_irqs,
	unsigned int first_hwirq,
	unsigned int first_irq,
	const struct irq_chip *chip,
	const struct irq_controller *controller
);

struct irq_domain *irq_domain_create_linear(
	const char *name,
	unsigned int nr_irqs,
	const struct irq_chip *chip,
	const struct irq_controller *controller
);

int irq_domain_set_chip(
	struct irq_domain *domain,
	const struct irq_chip *chip,
	const struct irq_controller *controller
);

int irq_domain_map(struct irq_domain *domain, unsigned int hwirq, unsigned int *irq);
void irq_domain_unmap(struct irq_domain *domain, unsigned int hwirq);
int irq_find_mapping(struct irq_domain *domain, unsigned int hwirq);
int irq_create_mapping(struct irq_domain *domain, unsigned int hwirq);

#endif
