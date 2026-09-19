#include <kernel/interrupt.h>
#include <kernel/printk.h>
#include <kernel/clock.h>
#include <kernel/panic.h>

#include <asm/idt.h>

#include <def/errno.h>
#include <mm/kheap.h>
#include <asm/ptrace.h>

extern struct irq_desc* irq_desc_get(int interrupt);
extern const struct irq_chip i8259A_chip;

void interrupt_eoi(int interrupt){
	i8259A_chip.eoi(interrupt);
}

void interrupt_mask(int interrupt){
	struct irq_desc* desc = irq_desc_get(interrupt);
	if(!desc) return;
	
	desc->masked = true;
	i8259A_chip.mask(interrupt);
}

void interrupt_unmask(int interrupt){
	struct irq_desc* desc = irq_desc_get(interrupt);
	if(!desc) return;
	
	desc->masked = false;
	i8259A_chip.unmask(interrupt);
}

int irq_register(enum irq_id irq, interrupt_handler_t handler, void *dev){
	int int_no = arch_irq_id_to_int_no(irq);
	if(int_no == IRQ_NOT_MAPPED){
		return -EINVAL;
	}

	return interrupt_register(int_no, handler, dev);
}

int irq_unregister(enum irq_id irq, interrupt_handler_t handler, void *dev){
	int int_no = arch_irq_id_to_int_no(irq);
	if(int_no == IRQ_NOT_MAPPED){
		return -EINVAL;
	}

	return interrupt_unregister(int_no, handler, dev);
}

void irq_mask(enum irq_id irq){
	int int_no = arch_irq_id_to_int_no(irq);
	if(int_no == IRQ_NOT_MAPPED){
		return;
	}

	interrupt_mask(int_no);
}

void irq_unmask(enum irq_id irq){
	int int_no = arch_irq_id_to_int_no(irq);
	if(int_no == IRQ_NOT_MAPPED){
		return;
	}

	interrupt_unmask(int_no);
}

int generic_handle_irq(struct irq_info* info){
	const struct irq_desc* desc = irq_desc_get(info->route.hw_line);
	if(!desc) return -ENOENT;	

	struct irq_handler_node* handler = desc->handlers;

	bool handled = !!(handler);

	while(handler) {
		info->device = handler->device;
		handler->handler(info);
		handler = handler->next;
	}

	if(info->cpu.exception){
		uint32_t interrupt = info->route.hw_line;	
		uintptr_t ip = regs_get_instruction_pointer(info->cpu.regs);
		const char* name = info->cpu.exception_name;

		if(handled){
			printk(
				"Received trap %d <0x%x>: '%s' at 0x%x\n",
				interrupt, interrupt, name, ip
			);
		}else if(!handled){
			printk(
				"\n\nUnhandled Exception %d <0x%x>: '%s' at 0x%x\n",
				interrupt, interrupt, name, ip
			);

			dump_regs(info->cpu.regs);

			panic("System Halted!");
		}
	}

	if(info->route.irq_id == IRQ_WR_TIMER){
		clockevent_fire();
	}

	return OK;
}

