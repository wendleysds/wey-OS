#include <kernel/interrupt.h>
#include <kernel/printk.h>
#include <kernel/clock.h>
#include <kernel/panic.h>

#include <asm/idt.h>

#include <def/errno.h>
#include <mm/kheap.h>
#include <asm/ptrace.h>

extern struct irq_desc* irq_desc_get(int interrupt);

void interrupt_eoi(int interrupt){
	struct irq_desc* desc = irq_desc_get(interrupt);
	if (desc && desc->controller && desc->controller->eoi) {
        desc->controller->eoi(desc);
    }
}

void interrupt_mask(int interrupt){
	struct irq_desc* desc = irq_desc_get(interrupt);
    if (desc && desc->chip && desc->chip->mask) {
        desc->masked = true;
        desc->chip->mask(desc);
    }
}

void interrupt_unmask(int interrupt){
	struct irq_desc* desc = irq_desc_get(interrupt);
    if (desc && desc->chip && desc->chip->unmask) {
        desc->masked = false;
        desc->chip->unmask(desc);
    }
}

extern const struct irq_chip i8259A_chip;
extern const struct irq_controller i8259A_controller;

int generic_handle_irq(struct irq_info* info){
	struct irq_desc* desc = irq_desc_get(info->hwirq);
	if(!desc) return -ENOENT;

	// tmp
	desc->chip = &i8259A_chip;
	desc->controller = &i8259A_controller;

	struct irq_handler_node* handler = desc->handlers;

	bool handled = !!(handler);

	while(handler) {
		info->device = handler->device;
		handler->handler(info);
		handler = handler->next;
	}

	if(info->cpu.exception){
		uint32_t interrupt = info->hwirq;
		uintptr_t ip = regs_get_instruction_pointer(info->cpu.regs);
		const char* name = info->cpu.exception_name;

		if(handled){
			printk(
				"Received trap %d <0x%x>: '%s' at 0x%x\n",
				interrupt, interrupt, name, ip
			);
		}else if(!handled){
			printk(
				"Unhandled Exception %d <0x%x>: '%s' at 0x%x\n",
				interrupt, interrupt, name, ip
			);

			dump_regs(info->cpu.regs);

			panic("System Halted");
		}
	}

	if (info->needs_eoi && desc->controller && desc->controller->eoi) {
        desc->controller->eoi(desc);
    }

	return OK;
}

