#include <arch/i386/idt.h>
#include <drivers/terminal.h>
#include <config/config.h>
#include <arch/io.h>
#include <lib/mem.h>
#include <core/kernel.h>

#include <stdint.h>

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
extern void* interrupt_pointer_table[TOTAL_INTERRUPTS];

struct InterruptDescriptor idt[TOTAL_INTERRUPTS];
struct IDTr_ptr idtr_ptr;

static INTERRUPT_CALLBACK_FUNCTION interrupt_callbacks[TOTAL_INTERRUPTS] = {0x0};

void _set_idt_gate(uint8_t interrupt_num, uint32_t base, uint16_t selector, uint8_t flags){
	struct InterruptDescriptor* desc = &idt[interrupt_num];
	desc->offset_1 = (uint32_t) base & 0xFFFF;
	desc->selector = selector;
	desc->zero = 0x0;
	desc->type_attributes = flags;
	desc->offset_2 = (uint32_t) (base >> 16) & 0xFFFF;
}

void _set_idt(uint8_t interrupt_num, void* address){
	_set_idt_gate(interrupt_num, (uint32_t)address, KERNEL_CODE_SELECTOR, 0x8E);
}

void init_idt(){
	memset(idt, 0x0, sizeof(idt));
	idtr_ptr.limit = sizeof(idt) - 1;
	idtr_ptr.base = (uintptr_t)&idt;

	for(int i = 0; i < 256; i++){
		_set_idt(i, interrupt_pointer_table[i]);
	}

	load_idt(&idtr_ptr);
}

void _print_frame(struct InterruptFrame* frame){
  terminal_write(TERMINAL_DEFAULT_COLOR, 
    "\nds 0x%x edi 0x%x esi 0x%x ebp 0x%x\n", 
    frame->ds, frame->edi, frame->esi, frame->ebp
  );
  terminal_write(TERMINAL_DEFAULT_COLOR, 
    "kernelesp 0x%x ebx 0x%x edx 0x%x ecx 0x%x\n", 
    frame->kernelesp, frame->ebx, frame->edx, frame->ecx
  );
  terminal_write(TERMINAL_DEFAULT_COLOR, 
    "eax 0x%x int_no 0x%x err_code 0x%x eip 0x%x\n", 
    frame->eax, frame->int_no, frame->err_code, frame->eip
  );
  terminal_write(TERMINAL_DEFAULT_COLOR, 
    "cs 0x%x eflags 0x%x useresp 0x%x ss 0x%x\n\n", 
    frame->cs, frame->eflags, frame->useresp, frame->ss
  );
}

void idt_register_callback(int interrupt, INTERRUPT_CALLBACK_FUNCTION callback){
	if(interrupt < 0 || interrupt >= TOTAL_INTERRUPTS){
		return;
	}

	interrupt_callbacks[interrupt] = callback;
}

void interrupt_handler(struct InterruptFrame* frame){
	
	kernel_registers();

	if(interrupt_callbacks[frame->int_no] != 0){
		interrupt_callbacks[frame->int_no](frame);
	}
	else{
		if(frame->int_no < 32){
			terminal_write(TERMINAL_DEFAULT_COLOR, 
					"\n\nUnhandled Exception %d <0x%x>: '%s' at 0x%x\n",
					frame->int_no, frame->int_no, _exceptionMessages[frame->int_no], frame->eip);

			_print_frame(frame);

			terminal_write(TERMINAL_DEFAULT_COLOR, "System Halted!\n");
			while(1);
		}
		else{
			terminal_write(TERMINAL_DEFAULT_COLOR, 
					"\n\nUnhandled Interrupt %d <0x%x> at 0x%x\n",
					frame->int_no, frame->int_no, frame->eip);
		}

		_print_frame(frame);
	}

	outb(0x20, 0x20);
}

