#include "idt.h"
#include "../terminal/terminal.h"
#include "../config.h"
#include "../io/io.h"
#include "../memory/memory.h"
#include "../kernel/kernel.h"

#include <stdint.h>

#define TOTAL_ISR_INTERRUPTS 32 // 0  - 31

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

extern void load_idt(struct IDTr_ptr* ptr);
extern void* isr_pointer_table[1];

struct InterruptDescriptor idt[TOTAL_ISR_INTERRUPTS];
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

	set_idt(0, isr_pointer_table[0]);

	load_idt(&idtr_ptr);
	enable_interrupts();
}

void print_frame(struct _InterruptFrame* frame){
  terminal_writef(TERMINAL_DEFAULT_COLOR, 
    "ds 0x%x edi 0x%x esi 0x%x ebp 0x%x\n", 
    frame->ds, frame->edi, frame->esi, frame->ebp
  );
  terminal_writef(TERMINAL_DEFAULT_COLOR, 
    "esp 0x%x ebx 0x%x edx 0x%x ecx 0x%x\n", 
    frame->esp, frame->ebx, frame->edx, frame->ecx
  );
  terminal_writef(TERMINAL_DEFAULT_COLOR, 
    "eax 0x%x int_no 0x%x err_code 0x%x eip 0x%x\n", 
    frame->eax, frame->int_no, frame->err_code, frame->eip
  );
  terminal_writef(TERMINAL_DEFAULT_COLOR, 
    "cs 0x%x eflags 0x%x useresp 0x%x ss 0x%x\n\n", 
    frame->cs, frame->eflags, frame->useresp, frame->ss
  );
}

void interrupt_handler(struct _InterruptFrame* frame){
	kernel_registers();

  terminal_write("\n\nInterruption:\n", TERMINAL_DEFAULT_COLOR);
  print_frame(frame);

	/*if(interrupt < 32){
		terminal_writef(TERMINAL_DEFAULT_COLOR, 
				"\n\nException <0x%x>: '%s' at 0x%x\n", 
				interrupt, _exceptionMessages[interrupt], frame->eip
		);
		outb(0x20, 0x20);
		while(1);
	}*/

	outb(0x20, 0x20);
}
