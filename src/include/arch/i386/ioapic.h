#ifndef _I386_IOAPIC_H
#define _I386_IOAPIC_H

#include <stdint.h>
#include <stdbool.h>
#include <kernel/interrupt.h>

int ioapic_init(uint32_t local_apic_id);
void ioapic_set_routing(uint32_t pin, uint8_t vector, bool edge_triggered, bool no_poll);
void ioapic_mask(uint32_t pin);
void ioapic_unmask(uint32_t pin);
void ioapic_mask_all(void);
void ioapic_send_eoi(int vector);

uint32_t ioapic_read(uint32_t reg);
void ioapic_write(uint32_t reg, uint32_t value);
int ioapic_irq_to_pin(uint32_t irq);
void ioapic_mask_irq(uint32_t irq);
void ioapic_unmask_irq(uint32_t irq);

extern const struct irq_chip ioapic_chip;
extern const struct irq_controller ioapic_controller;

#endif