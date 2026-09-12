#ifndef _MMIO_H
#define _MMIO_H

#include <sys/types.h>

#define __iomem

static inline uint8_t mmio_read8(const volatile vaddr_t __iomem addr){
	return *(volatile uint8_t *)(addr);
}

static inline uint16_t mmio_read16(const volatile vaddr_t __iomem addr){
	return *(volatile uint16_t *)(addr);
}

static inline uint32_t mmio_read32(const volatile vaddr_t __iomem addr){
	return *(volatile uint32_t *)(addr);
}

static inline void mmio_write8(volatile vaddr_t __iomem addr, uint8_t val){
	*(volatile uint8_t *)(addr) = val;
}

static inline void mmio_write16(volatile vaddr_t __iomem addr, uint16_t val){
	*(volatile uint16_t *)(addr) = val;
}

static inline void mmio_write32(volatile vaddr_t __iomem addr, uint32_t val){
	*(volatile uint32_t *)(addr) = val;
}

#endif
