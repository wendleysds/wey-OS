#include <kernel/interrupt.h>
#include <def/errno.h>
#include <mm/kheap.h>

#include <asm/idt.h>

// TODO: change to a rbtree
static struct irq_desc irq_table[TOTAL_INTERRUPTS];

int interrupt_register(int interrupt, interrupt_handler_t handler, void *dev){
	int hwirq = arch_irq_to_hwline(interrupt);
	if(hwirq < 0 || hwirq >= TOTAL_INTERRUPTS){
		return -EINVAL;
	}

	struct irq_handler_node* node = kmalloc(sizeof(struct irq_handler_node));
	if(!node){
		return -ENOMEM;
	}

	struct irq_desc *desc = &irq_table[hwirq];

	desc->irq = interrupt;
	desc->hwirq = hwirq;

	desc->masked = false;

    node->handler = handler;
    node->device = dev;
    node->next = desc->handlers;

	desc->handlers = node;

	return OK;
}

int interrupt_unregister(int interrupt, interrupt_handler_t handler, void *dev){
	int hwirq = arch_irq_to_hwline(interrupt);
	if(hwirq < 0 || hwirq >= TOTAL_INTERRUPTS){
		return -EINVAL;
	}

	struct irq_desc *desc = &irq_table[hwirq];
	struct irq_handler_node *cur, *prev = NULL;

	cur = desc->handlers;
	while(cur){
		if(cur->device == dev && cur->handler == handler){
			if(prev){
				prev->next = cur->next;
			}else{
				desc->handlers = cur->next;
			}

			kfree(cur);
			return OK;
		}

		prev = cur;
		cur = cur->next;
	}

	return -ENOENT;
}

struct irq_desc* irq_desc_get(int interrupt) {
	int hwirq = arch_irq_to_hwline(interrupt);
	if(hwirq < 0 || hwirq >= TOTAL_INTERRUPTS){
		return NULL;
	}

	return &irq_table[hwirq];
}

void interrupt_set_chip(int interrupt, struct irq_chip* chip){
	struct irq_desc *desc = irq_desc_get(interrupt);
	if(desc){
		desc->chip = chip;
	}
}

void interrupt_set_controller(int interrupt, struct irq_controller* controller){
	struct irq_desc *desc = irq_desc_get(interrupt);
	if(desc){
		desc->controller = controller;
	}
}

const struct irq_chip* interrupt_get_chip(int interrupt){
	struct irq_desc *desc = irq_desc_get(interrupt);
	if(desc){
		return desc->chip;
	}
	return NULL;
}

const struct irq_controller* interrupt_get_controller(int interrupt){
	struct irq_desc *desc = irq_desc_get(interrupt);
	if(desc){
		return desc->controller;
	}
	return NULL;
}