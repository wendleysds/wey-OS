#ifndef _X86_INTERRUPTS_H
#define _X86_INTERRUPTS_H

#include <stdint.h>

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

void idt_init();
void idt_set_gate(uint8_t interrupt_num, uint32_t base, uint16_t selector, uint8_t flags);

#endif
