#ifndef _X86_PIC_H
#define _X86_PIC_H

#include <stdint.h>

void pic_init(uint32_t frequency);
void pic_send_eoi(uint8_t irq);
void pic_disable();

void IRQ_set_mask(uint8_t IRQline);
void IRQ_clear_mask(uint8_t IRQline);

uint16_t pic_get_irr(void);
uint16_t pic_get_isr(void);

#endif
