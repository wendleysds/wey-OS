#include <wey/interrupt.h>
#include <def/config.h>
#include <def/init.h>
#include <lib/string.h>
#include <arch/i386/pic.h>
#include <drivers/terminal.h>

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

static irq_handler_t interrupt_callbacks[TOTAL_INTERRUPTS] = {0x0};

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
	memset(idt, 0x0, sizeof(idt));
	idtr_ptr.limit = sizeof(idt) - 1;
	idtr_ptr.base = (uintptr_t)&idt;

	for(int i = 0; i < TOTAL_INTERRUPTS; i++){
		idt_set_gate(
			i, 
			(uint32_t)interrupt_pointer_table[i], 
			KERNEL_CODE_SELECTOR, 
			(IDT_PRESENT | IDT_DPL0 | IDT_TYPE_INT_GATE32)
		);
	}

	_idt_load(&idtr_ptr);
	interrupts_enable();
}

void interrupts_enable(){
	__asm__ volatile ("sti");
}

void interrupts_disable(){
	__asm__ volatile ("cli");
}

void interrupt_register(int interrupt, irq_handler_t handler){
	if(interrupt < 0 || interrupt >= TOTAL_INTERRUPTS){
		return;
	}
	
	interrupt_callbacks[interrupt] = handler;
}

static void _print_frame(struct registers* regs){
	terminal_write(
		"eax 0x%x ebx 0x%x ecx 0x%x edx 0x%x\n",
		regs->ax, regs->bx, regs->cx, regs->dx
	);

	terminal_write( 
		"esi 0x%x edi 0x%x esp 0x%x ebp 0x%x\n",
		regs->si, regs->di, regs->sp, regs->bp
	);

	terminal_write(
		"ip 0x%x cs 0x%x ss 0x%x\n",
		regs->ip, regs->cs, regs->ss
	);

	terminal_write(
		"eflags 0x%x kesp 0x%x\n",
		regs->flags, regs->ksp
	);

	terminal_write(
		"int 0x%x err 0x%x\n",
		regs->int_no, regs->err_code
	);
}

void __cdecl interrupt_handler(struct registers* regs){
	kernel_registers();

	int interrupt = regs->int_no;

	if(interrupt_callbacks[interrupt] != 0){
		interrupt_callbacks[interrupt](regs);
	}
	else if(interrupt < 32){
		terminal_write(
			"\n\nUnhandled Exception %d <0x%x>: '%s' at 0x%x\n",
			interrupt, interrupt, exception_messages[interrupt], regs->ip);

		_print_frame(regs);

		terminal_write("System Halted!\n");
		while(1){
			__asm__ volatile ("hlt");
		}
	}

	pic_send_eoi(interrupt);
	user_registers();
}