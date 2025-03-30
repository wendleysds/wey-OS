#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>

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

void init_idt();
void set_idt(uint8_t interrupt_num, void* address);
void enable_interrupts();
void disable_interrupts();

#endif
