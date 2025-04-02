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

struct InterruptFrame {
  uint32_t ds;
  uint32_t edi, esi, ebp, kernelesp, ebx, edx, ecx, eax;
  uint32_t int_no, err_code;
  uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed));

void init_idt();
void set_idt(uint8_t interrupt_num, void* address);
void enable_interrupts();
void disable_interrupts();

#endif
