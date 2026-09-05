#ifndef _MMIO_H
#define _MMIO_H

#include <stdint.h>

static inline uint8_t mmio_read8(uintptr_t addr){
	return *(volatile uint8_t *)(addr);
}

static inline uint16_t mmio_read16(uintptr_t addr){
	return *(volatile uint16_t *)(addr);
}

static inline uint32_t mmio_read32(uintptr_t addr){
	return *(volatile uint32_t *)(addr);
}

static inline void mmio_write8(uintptr_t addr, uint8_t val){
	*(volatile uint8_t *)(addr) = val;
}

static inline void mmio_write16(uintptr_t addr, uint16_t val){
	*(volatile uint16_t *)(addr) = val;
}

static inline void mmio_write32(uintptr_t addr, uint32_t val){
	*(volatile uint32_t *)(addr) = val;
}

#endif
