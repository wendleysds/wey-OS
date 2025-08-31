#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>

#define IRQ(x) (0x20 + x)

#define IDT_PRESENT 0x80
#define IDT_DPL0    0x00
#define IDT_DPL1    0x20
#define IDT_DPL2    0x40
#define IDT_DPL3    0x60

#define IDT_TYPE_TASK_GATE   0x5
#define IDT_TYPE_INT_GATE16  0x6
#define IDT_TYPE_TRAP_GATE16 0x7
#define IDT_TYPE_INT_GATE32  0xE
#define IDT_TYPE_TRAP_GATE32 0xF

struct InterruptDescriptor{
	uint16_t offset_1;
	uint16_t selector;
	uint8_t zero;
	uint8_t type_attributes;
	uint16_t offset_2;
}__attribute__((packed));

struct IDTr_ptr{
	uint16_t limit;
	uint32_t base;
}__attribute__((packed));

struct InterruptFrame {	
	uint32_t edi, esi, ebp, kesp;
	uint32_t ebx, edx, ecx, eax;
	uint32_t int_no, err_code;
	uint32_t eip, cs, eflags;
	uint32_t esp, ss;
} __attribute__((packed));

typedef void(*INTERRUPT_CALLBACK_FUNCTION)(struct InterruptFrame* frame);

void init_idt();
void enable_interrupts();
void disable_interrupts();
void idt_register_callback(int interrupt, INTERRUPT_CALLBACK_FUNCTION callback);

#endif
