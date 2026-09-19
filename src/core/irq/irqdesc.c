#include <kernel/interrupt.h>
#include <def/errno.h>
#include <mm/kheap.h>

// TODO: change to a rbtree
static struct irq_desc irq_table[TOTAL_INTERRUPTS];

int interrupt_register(int interrupt, interrupt_handler_t handler, void *dev){
	if(interrupt < 0 || interrupt >= TOTAL_INTERRUPTS){
		return -EINVAL;
	}
	
	struct irq_handler_node* node = kmalloc(sizeof(struct irq_handler_node));
	if(!node){
		return -ENOMEM;
	}

    node->handler = handler;
    node->device = dev;
    node->next = irq_table[interrupt].handlers;

    irq_table[interrupt].handlers = node;
	irq_table[interrupt].hw_line = interrupt;

	return OK;
}

int interrupt_unregister(int interrupt, interrupt_handler_t handler, void *dev){
	if(interrupt < 0 || interrupt >= TOTAL_INTERRUPTS){
		return -EINVAL;
	}

	struct irq_handler_node *cur, *prev = NULL;
	cur = irq_table[interrupt].handlers;

	while(cur){
		if(cur->device == dev && cur->handler == handler){
			if(prev){
				prev->next = cur->next;
			}else{
				irq_table[interrupt].handlers = cur->next;
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
	if(interrupt < 0 || interrupt >= TOTAL_INTERRUPTS){
		return NULL;
	}

	return &irq_table[interrupt];
}