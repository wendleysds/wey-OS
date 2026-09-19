#include <kernel/interrupt.h>
#include <kernel/init.h>

#include <lib/list.h>

static LIST_HEAD(irqchips);

void __init irqchip_register(struct irq_chip* chip){
	INIT_LIST_HEAD(&chip->node);
	list_add(&chip->node, &irqchips);
}