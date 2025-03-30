#include "idt.h"
#include "../terminal/terminal.h"
#include "../config.h"
#include "../io/io.h"
#include "../memory/memory.h"
#include "../kernel/kernel.h"

#include <stdint.h>

struct _InterruptFrame {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed));

const char* _exceptionMessages[] = {
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
  "Coprocessor Fault",
  "Alignment Fault",
  "Machine Check", 
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
  "Reserved",
	"Reserved",
	"Reserved"
};

extern void load_idt(struct IDTr_ptr* ptr);

struct InterruptDescriptor idt[TOTAL_INTERRUPTS];
struct IDTr_ptr idtr_ptr;

void _set_idt_gate(uint8_t interrupt_num, uint32_t base, uint16_t selector, uint8_t flags){
	struct InterruptDescriptor* desc = &idt[interrupt_num];
	desc->offset_1 = base & 0xFFFF;
	desc->selector = selector;
	desc->zero = 0x0;
	desc->type_attributes = flags;
	desc->offset_2 = (base >> 16) & 0xFFFF;
}

void set_idt(uint8_t interrupt_num, void* address){
	_set_idt_gate(interrupt_num, (uint32_t)address, KERNEL_CODE_SELECTOR, 0xEE);
}

void idt_zero(){
	terminal_writef(TERMINAL_DEFAULT_COLOR, "\n\nException <0x0>: '%s'\n", _exceptionMessages[0]);
	outb(0x20, 0x20);
	while(1);
}

void init_idt(){
	memset(idt, 0x0, sizeof(idt));
	idtr_ptr.limit = sizeof(idt) - 1;
	idtr_ptr.base = (uintptr_t)&idt[0];

	set_idt(0, idt_zero);

	load_idt(&idtr_ptr);
	enable_interrupts();
}

void interrupt_handler(int interrupt, struct _InterruptFrame* frame){
	kernel_registers();

	if(interrupt < 32){
		terminal_writef(TERMINAL_DEFAULT_COLOR, 
				"\n\nException <0x%x>: '%s' at 0x%x\n", 
				interrupt, _exceptionMessages[interrupt], frame->eip
		);
		outb(0x20, 0x20);
		while(1);
	}

	outb(0x20, 0x20);
}
