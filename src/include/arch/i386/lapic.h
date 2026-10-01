#ifndef _I386_LAPIC_H
#define _I386_LAPIC_H

#include <stdint.h>

typedef void (*wait_10ms_func_t)(void);

int lapic_init(int frequency, wait_10ms_func_t calibrator);
void lapic_enable(void);
void lapic_disable(void);

void lapic_mask(uint32_t lvt_reg);
void lapic_unmask(uint32_t lvt_reg);
void lapic_mask_local_sources(void);

void lapic_eoi(void);
uint32_t lapic_get_id(void);

#endif