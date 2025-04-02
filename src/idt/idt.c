#include "idt.h"
#include "../terminal/terminal.h"
#include "../config.h"
#include "../io/io.h"
#include "../memory/memory.h"
#include "../kernel/kernel.h"

#include <stdint.h>

#define TOTAL_ISR_INTERRUPTS 32 // 0 - 31
#define TOTAL_IRQ_INTERRUPTS 16 // 0 - 15

const char* _exceptionMessages[TOTAL_ISR_INTERRUPTS] = {
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

extern void* isr_pointer_table[TOTAL_ISR_INTERRUPTS];
extern void* irq_pointer_table[TOTAL_IRQ_INTERRUPTS];

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
	_set_idt_gate(interrupt_num, (uint32_t)address, KERNEL_CODE_SELECTOR, 0x8E);
}

void init_idt(){
	memset(idt, 0x0, sizeof(idt));
	idtr_ptr.limit = sizeof(idt) - 1;
	idtr_ptr.base = (uintptr_t)&idt[0];

  // 0 -> 31
	for(int i = 0; i < TOTAL_ISR_INTERRUPTS; i++){
		set_idt(i, isr_pointer_table[i]);
	}

  // 32 -> 47          32           i < 48                 16 + 32
  for(int i = TOTAL_ISR_INTERRUPTS; i < (TOTAL_ISR_INTERRUPTS + TOTAL_IRQ_INTERRUPTS); i++){
    set_idt(i, irq_pointer_table[i]);
  }

	load_idt(&idtr_ptr);
	enable_interrupts();
}

void print_frame(struct InterruptFrame* frame){
  terminal_writef(TERMINAL_DEFAULT_COLOR, 
    "\nds 0x%x edi 0x%x esi 0x%x ebp 0x%x\n", 
    frame->ds, frame->edi, frame->esi, frame->ebp
  );
  terminal_writef(TERMINAL_DEFAULT_COLOR, 
    "kernelesp 0x%x ebx 0x%x edx 0x%x ecx 0x%x\n", 
    frame->kernelesp, frame->ebx, frame->edx, frame->ecx
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

void isr_handler(struct InterruptFrame* frame){
	
	if(frame->int_no < 32)
		terminal_writef(TERMINAL_DEFAULT_COLOR, 
			"\n\nUnhandled Exception <%d>: '%s' at 0x%x\n", 
			frame->int_no, _exceptionMessages[frame->int_no], frame->eip);
	else
		terminal_writef(TERMINAL_DEFAULT_COLOR, 
			"\n\nUnhandled Interrupt <%d>: '%s' at 0x%x\n", 
			frame->int_no, _exceptionMessages[frame->int_no], frame->eip);

	print_frame(frame);

	outb(0x20, 0x20);

	terminal_write("System Halted!\n", TERMINAL_DEFAULT_COLOR);

	while(1);
}

void *irq_handlers[TOTAL_IRQ_INTERRUPTS] = {0x0};

void irq_add_handler (int irq, void (*handler)(struct InterruptRegisters *r)){
  if(irq < 0 || irq > (TOTAL_IRQ_INTERRUPTS - 1)){
    irq_handlers[irq] = handler;
  }   
}

void irq_remove_handler(int irq){
  if(irq < 0 || irq > (TOTAL_IRQ_INTERRUPTS - 1)){
    irq_handlers[irq] = 0;
  }   
}

void irq_handler(struct InterruptFrame* frame){
  void (*handler)(struct InterruptFrame* frame);

  int handlerIqr = frame->int_no - 32;

  if(handlerIqr < 0 || handlerIqr > (TOTAL_IRQ_INTERRUPTS - 1)){
    terminal_writef(TERMINAL_DEFAULT_COLOR, "\nInvalid Interrupt Request: '%d'\n", handlerIqr);
    print_frame(frame);
    outb(0x20, 0x20);
    return;
  }

  handler = irq_handlers[handlerIqr];

  terminal_write("\nInterrupt Request:\n", TERMINAL_DEFAULT_COLOR);
  print_frame(frame);

  if (handler){
    handler(frame);
  }

  if (frame->int_no >= 40){
    outb(0xA0, 0x20);
  }

  outb(0x20, 0x20);
}
