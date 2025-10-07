#include <arch/i386/idt.h>
#include <arch/i386/pic.h>
#include <drivers/terminal.h>
#include <def/config.h>
#include <io/ports.h>
#include <lib/string.h>
#include <wey/panic.h>
#include <wey/sched.h>
#include <def/compile.h>

#include <stdint.h>

/*
 * IDT & IQR Configurator and Loader
 */

static const char* _exceptionMessages[] = {
	"Division By Zero",
	"Debug",
	"Non Maskable Interrupt",
	"Breakpoint",
	"Into Detected Overflow",
	"Out of Bounds",
	"Invalid Opcode",
	"No Coprocessor",
	"Double fault",
	"Coprocessor Segment Overrun",
	"Bad TSS",
	"Segment not present",
	"Stack fault",
	"General protection fault",
	"Page fault",
	"Unknown Interrupt",
	"x87 Floating-Point",
	"Alignment Fault",
	"Machine Check", 
	"SIMD Floating-Point",
	"Vitualization",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved"
};

extern void kernel_registers();
extern void user_registers();

static void _print_frame(struct InterruptFrame* frame);

extern void load_idt(struct IDTr_ptr* ptr);
extern void* interrupt_pointer_table[TOTAL_INTERRUPTS];

struct InterruptDescriptor idt[TOTAL_INTERRUPTS];
struct IDTr_ptr idtr_ptr;

static INTERRUPT_CALLBACK_FUNCTION interrupt_callbacks[TOTAL_INTERRUPTS] = {0x0};

static void _set_idt_gate(uint8_t interrupt_num, uint32_t base, uint16_t selector, uint8_t flags){
	struct InterruptDescriptor* desc = &idt[interrupt_num];
	desc->offset_1 = base & 0xFFFF;
	desc->selector = selector;
	desc->zero = 0x0;
	desc->type_attributes = flags;
	desc->offset_2 = (base >> 16) & 0xFFFF;
}

void _set_idt(uint8_t interrupt_num, void* address, uint8_t flags){
	_set_idt_gate(interrupt_num, (uint32_t)address, KERNEL_CODE_SELECTOR, flags);
}

void init_idt(){
	memset(idt, 0x0, sizeof(idt));
	idtr_ptr.limit = sizeof(idt) - 1;
	idtr_ptr.base = (uintptr_t)&idt;

	for(int i = 0; i < TOTAL_INTERRUPTS; i++){
		_set_idt(i, interrupt_pointer_table[i], IDT_PRESENT | IDT_DPL0 | IDT_TYPE_INT_GATE32);
	}

	load_idt(&idtr_ptr);
	enable_interrupts();
}

static void _print_frame(struct InterruptFrame* frame){
	terminal_write(
		"eax 0x%x ebx 0x%x ecx 0x%x edx 0x%x\n",
		frame->eax, frame->ebx, frame->ecx, frame->edx
	);

	terminal_write( 
		"esi 0x%x edi 0x%x esp 0x%x ebp 0x%x\n",
		frame->esi, frame->edi, frame->esp, frame->ebp
	);

	terminal_write(
		"ip 0x%x cs 0x%x ss 0x%x\n",
		frame->eip, frame->cs, frame->ss
	);

	terminal_write(
		"eflags 0x%x kesp 0x%x\n",
		frame->eflags, frame->kesp
	);

	terminal_write(
		"int 0x%x err 0x%x\n",
		frame->int_no, frame->err_code
	);
}

void idt_register_callback(int interrupt, INTERRUPT_CALLBACK_FUNCTION callback){
	if(interrupt < 0 || interrupt >= TOTAL_INTERRUPTS){
		return;
	}

	interrupt_callbacks[interrupt] = callback;
}

void __cdecl interrupt_handler(struct InterruptFrame* frame){
	kernel_registers();

	int interrupt = frame->int_no;

	if(interrupt_callbacks[interrupt] != 0){
		interrupt_callbacks[interrupt](frame);
	}
	else if(interrupt < 32){
		terminal_write(
			"\n\nUnhandled Exception %d <0x%x>: '%s' at 0x%x\n",
			interrupt, interrupt, _exceptionMessages[interrupt], frame->eip);

		_print_frame(frame);

		terminal_write("System Halted!\n");
		while(1){
			__asm__ volatile ("hlt");
		}
	}

	pic_send_eoi(interrupt);
	user_registers();
}

