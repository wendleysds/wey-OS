#include "idt.h"
#include "../terminal/terminal.h"
#include "../config.h"
#include "../io/io.h"
#include "../memory/memory.h"

#include <stdint.h>

struct InterruptDescriptor id[TOTAL_INTERRUPTS];
struct IDTr_ptr idtr_ptr;

extern void load_idt(struct IDTr_ptr* ptr);

void set_idt_gate(uint8_t interrupt_num, uint32_t base, uint16_t selector, uint8_t flags){
	id[interrupt_num].offset_1 = base & 0xFFFF;
	id[interrupt_num].selector = selector;
	id[interrupt_num].zero = 0x0;
	id[interrupt_num].type_attributes = flags;
	id[interrupt_num].offset_2 = (base >> 16) & 0xFFFF;
}

void init_idt(){
	memset(id, 0x0, sizeof(id));
	idtr_ptr.limit = sizeof(id) - 1;
	idtr_ptr.base = (uint32_t) id;

	load_idt(&idtr_ptr);
}

