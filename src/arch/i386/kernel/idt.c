#include <kernel/sched.h>
#include <kernel/interrupt.h>
#include <kernel/printk.h>
#include <mm/kheap.h>
#include <asm/idt.h>
#include <def/err.h>
#include <lib/string.h>
#include <arch/i386/pic.h>

static const char* exception_messages[] = {
	"Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
	"Into Detected Overflow", "Out of Bounds", "Invalid Opcode", "No Coprocessor",
	"Double fault", "Coprocessor Segment Overrun", "Bad TSS", "Segment not present",
	"Stack fault", "General protection fault", "Page fault", "Unknown Interrupt",
	"x87 Floating-Point", "Alignment Fault", "Machine Check", "SIMD Floating-Point",
	"Vitualization",
	"Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved", "Reserved"
};

extern void* interrupt_pointer_table[TOTAL_INTERRUPTS];

static struct InterruptDescriptor idt[TOTAL_INTERRUPTS];
static struct IDTr_ptr idtr_ptr;

static struct irq_desc irq_table[TOTAL_INTERRUPTS];

extern void kernel_registers();
extern void user_registers();

volatile int need_resched = 0;

static inline void _idt_load(struct IDTr_ptr* ptr){
	__asm__ volatile ("lidt (%0)" : : "r"(ptr));
}

void idt_set_gate(uint8_t interrupt_num, uint32_t base, uint16_t selector, uint8_t flags){
	struct InterruptDescriptor* desc = &idt[interrupt_num];
	desc->offset_1 = base & 0xFFFF;
	desc->selector = selector;
	desc->zero = 0x0;
	desc->type_attributes = flags;
	desc->offset_2 = (base >> 16) & 0xFFFF;
}

__init void idt_init(){
	memset(irq_table, 0x0, sizeof(irq_table));
	memset(idt, 0x0, sizeof(idt));
	idtr_ptr.limit = sizeof(idt) - 1;
	idtr_ptr.base = (uintptr_t)&idt;

	for(int i = 0; i < TOTAL_INTERRUPTS; i++){
		idt_set_gate(
			i, 
			(uint32_t)interrupt_pointer_table[i], 
			GDT_KERNEL_CODE, 
			(IDT_PRESENT | IDT_DPL0 | IDT_TYPE_INT_GATE32)
		);
	}

	_idt_load(&idtr_ptr);
	printk("Setup: idt: loaded \"%#lx\".\n", &idtr_ptr);
}

void interrupts_enable(){
	__asm__ volatile ("sti");
}

void interrupts_disable(){
	__asm__ volatile ("cli");
}

int interrupt_register(int interrupt, interrupt_handler_t handler, void *dev){
	if(interrupt < 0 || interrupt >= TOTAL_INTERRUPTS){
		return INVALID_ARG;
	}
	
	struct irq_handler_node* node = kmalloc(sizeof(struct irq_handler_node));
	if(!node){
		return NO_MEMORY;
	}

    node->handler = handler;
    node->device = dev;

    node->next = irq_table[interrupt].handlers;
    irq_table[interrupt].handlers = node;

	return OK;
}

int interrupt_unregister(int interrupt, interrupt_handler_t handler, void *dev){
	if(interrupt < 0 || interrupt >= TOTAL_INTERRUPTS){
		return INVALID_ARG;
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

	return NOT_FOUND;
}

static int irq_id_to_int_no(irq_id_t irq){
	int irq_start = 0x20;
	switch (irq) {
		case IRQ_WR_TIMER: return      irq_start + 0;
		case IRQ_KEYBOARD: return      irq_start + 1;
		case IRQ_ATA_PRIMARY: return   irq_start + 14;
		case IRQ_ATA_SECONDARY: return irq_start + 15;
		default: return IRQ_NOT_MAPPED;
	}
}

int irq_register(irq_id_t irq, interrupt_handler_t handler, void *dev){
	int int_no = irq_id_to_int_no(irq);
	if(int_no == IRQ_NOT_MAPPED){
		return INVALID_ARG;
	}

	return interrupt_register(int_no, handler, dev);
}

int irq_unregister(irq_id_t irq, interrupt_handler_t handler, void *dev){
	int int_no = irq_id_to_int_no(irq);
	if(int_no == IRQ_NOT_MAPPED){
		return INVALID_ARG;
	}

	return interrupt_unregister(int_no, handler, dev);
}

void irq_mask(irq_id_t irq){
	int int_no = irq_id_to_int_no(irq);
	if(int_no == IRQ_NOT_MAPPED){
		return;
	}

	interrupt_mask(int_no);
}

void irq_unmask(irq_id_t irq){
	int int_no = irq_id_to_int_no(irq);
	if(int_no == IRQ_NOT_MAPPED){
		return;
	}

	interrupt_unmask(int_no);
}

static void _print_frame(struct registers* regs){
	printk(
		"eax 0x%x ebx 0x%x ecx 0x%x edx 0x%x\n",
		regs->ax, regs->bx, regs->cx, regs->dx
	);

	printk( 
		"esi 0x%x edi 0x%x esp 0x%x ebp 0x%x\n",
		regs->si, regs->di, regs->sp, regs->bp
	);

	printk(
		"ip 0x%x cs 0x%x ss 0x%x\n",
		regs->ip, regs->cs, regs->ss
	);

	printk(
		"eflags 0x%x kesp 0x%x\n",
		regs->flags, regs->ksp
	);

	printk(
		"int 0x%x err 0x%x\n",
		regs->int_no, regs->err_code
	);
}

static void _build_irq_info(struct irq_info* info, struct registers* regs){
	info->cpu.cpu_id = 0;
	info->cpu.regs = regs;
	info->cpu.from_user = regs_is_user_mode(regs);

	irq_id_t irq_id = IRQ_NOT_MAPPED;
	switch (regs->int_no) {
		case 0x20: irq_id = IRQ_WR_TIMER; break;
		case 0x20 + 1: irq_id = IRQ_KEYBOARD; break;
		case 0x20 + 14: irq_id = IRQ_ATA_PRIMARY; break;
		case 0x20 + 15: irq_id = IRQ_ATA_SECONDARY; break;
		default:break;
	}

	info->route.hw_line = regs->int_no;
	info->route.irq_id = irq_id;
	info->needs_eoi = (regs->int_no > 0x20 && !info->cpu.from_user);
}

void __cdecl interrupt_handler(struct registers* regs){
	kernel_registers();

	int interrupt = regs->int_no;

	if(likely(current))
		current->regs = *regs;

	struct irq_handler_node* h = irq_table[interrupt].handlers;

	struct irq_info info;
	_build_irq_info(&info, regs);

	char handled = !!(h);

	while(h) {
		info.device = h->device;
		h->handler(&info);
		h = h->next;
	}

	if(interrupt < 32){
		if(handled){
			printk(
				"Received trap %d <0x%x>: '%s' at 0x%x\n",
				interrupt, interrupt, exception_messages[interrupt], regs->ip);
		}else if(!handled){
			printk(
				"\n\nUnhandled Exception %d <0x%x>: '%s' at 0x%x\n",
				interrupt, interrupt, exception_messages[interrupt], regs->ip);

			_print_frame(regs);

			printk("System Halted!\n");
			while(1){
				__asm__ volatile ("hlt");
			}

			__builtin_unreachable();
		}
	}

	if(info.route.irq_id == IRQ_WR_TIMER)
		need_resched = 1;

	if(likely(current))
		*regs = current->regs;
}

void interrupt_eoi(uint8_t interrupt){
	pic_send_eoi(interrupt);
}

void interrupt_mask(uint8_t interrupt){
	IRQ_set_mask(interrupt);
}

void interrupt_unmask(uint8_t interrupt){
	IRQ_clear_mask(interrupt);
}
