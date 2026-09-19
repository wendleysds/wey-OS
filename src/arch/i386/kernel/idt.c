#include <kernel/sched.h>
#include <kernel/clock.h>
#include <kernel/interrupt.h>
#include <kernel/printk.h>
#include <kernel/stacktrace.h>
#include <mm/kheap.h>
#include <asm/idt.h>
#include <def/errno.h>
#include <lib/string.h>

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

int arch_irq_id_to_int_no(enum irq_id irq){
	int irq_start = 0x20;
	switch (irq) {
		case IRQ_WR_TIMER: return      irq_start + 0;
		case IRQ_KEYBOARD: return      irq_start + 1;
		case IRQ_ATA_PRIMARY: return   irq_start + 14;
		case IRQ_ATA_SECONDARY: return irq_start + 15;
		default: return IRQ_NOT_MAPPED;
	}
}

static void _build_irq_info(struct irq_info* info, struct registers* regs){
	info->cpu.cpu_id = 0;
	info->cpu.regs = regs;
	info->cpu.from_user = regs_is_user_mode(regs);

	if(regs->int_no < 0x20){
		info->cpu.exception = true;
		info->cpu.exception_name = exception_messages[regs->int_no];
	}

	enum irq_id irq_id = IRQ_NOT_MAPPED;
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

asmlinkage void arch_handle_irq(struct registers* regs){
	if(likely(current))
		current->regs = *regs;

	struct irq_info info;
	memset(&info, 0x0, sizeof(info));

	_build_irq_info(&info, regs);

	generic_handle_irq(&info);

	if(likely(current)){
		*regs = current->regs;
	}
}
