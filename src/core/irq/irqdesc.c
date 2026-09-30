#include <kernel/interrupt.h>
#include <kernel/init.h>
#include <def/errno.h>
#include <mm/kheap.h>
#include <lib/string.h>
#include <asm/idt.h>

static struct irq_desc irq_table[TOTAL_INTERRUPTS];

struct irq_desc* irq_desc_get_by_hwirq(int hwirq) {
	if (hwirq < 0 || hwirq >= TOTAL_INTERRUPTS) {
		return NULL;
	}
	return &irq_table[hwirq];
}

struct irq_desc* irq_to_desc(int irq) {
	if (irq < 0 || irq >= TOTAL_INTERRUPTS) {
		return NULL;
	}

	struct irq_domain *domain = irq_domain_get_default();
	if (domain) {
		if (irq >= (int)domain->irq_base && irq < (int)(domain->irq_base + domain->nr_irqs)) {
			int hwirq = domain->hwirq_base + (irq - domain->irq_base);
			if (hwirq >= 0 && hwirq < TOTAL_INTERRUPTS) {
				return &irq_table[hwirq];
			}
		}
	}

	if (irq >= 0 && irq < TOTAL_INTERRUPTS) {
		return &irq_table[irq];
	}

	return NULL;
}

struct irq_desc* irq_desc_get(int interrupt) {
	if (interrupt >= 0x20 && interrupt < TOTAL_INTERRUPTS) {
		return &irq_table[interrupt];
	}
	return irq_to_desc(interrupt);
}

int interrupt_register(int interrupt, interrupt_handler_t handler, void *dev) {
	struct irq_desc *desc = irq_to_desc(interrupt);
	if (!desc) {
		return -EINVAL;
	}

	struct irq_handler_node *node = kmalloc(sizeof(struct irq_handler_node));
	if (!node) {
		return -ENOMEM;
	}

	desc->irq = interrupt;
	desc->masked = false;

	node->handler = handler;
	node->device = dev;
	node->next = desc->handlers;

	desc->handlers = node;

	// If chip has unmask, unmask the IRQ
	if (desc->chip && desc->chip->unmask) {
		desc->chip->unmask(desc);
	}

	return OK;
}

int interrupt_unregister(int interrupt, interrupt_handler_t handler, void *dev) {
	struct irq_desc *desc = irq_to_desc(interrupt);
	if (!desc) {
		return -EINVAL;
	}

	struct irq_handler_node *cur = desc->handlers;
	struct irq_handler_node *prev = NULL;

	while (cur) {
		if (cur->device == dev && cur->handler == handler) {
			if (prev) {
				prev->next = cur->next;
			} else {
				desc->handlers = cur->next;
			}

			kfree(cur);

			// If no more handlers, mask the IRQ
			if (!desc->handlers && desc->chip && desc->chip->mask) {
				desc->masked = true;
				desc->chip->mask(desc);
			}

			return OK;
		}

		prev = cur;
		cur = cur->next;
	}

	return -ENOENT;
}

int __init irqdesc_init(void (*callback)(int hwirq, struct irq_desc* desc)) {
	memset(irq_table, 0, sizeof(irq_table));
	for (int i = 0; i < TOTAL_INTERRUPTS; i++) {
		if(callback) {
			callback(i, &irq_table[i]);
		} else {
			irq_table[i].hwirq = i;
			irq_table[i].irq = i;
			irq_table[i].masked = true;
		}
	}
	return OK;
}