#ifndef _IO_REGION_H
#define _IO_REGION_H

#define PORT_NEW_API

#include <io/ports.h>
#include <io/mmio.h>

typedef enum {
	IO_TYPE_MMIO,
	IO_TYPE_PIO
} io_type_t;

typedef struct {
	io_type_t type;
	union {
		uintptr_t mmio_base;
		uint16_t  pio_base;
	};
} io_region_t;

static inline uint8_t io_read8(const io_region_t *io, uintptr_t offset){
	if(io->type == IO_TYPE_PIO){
		return port_read8(io->pio_base + offset);
	}

	return mmio_read8(io->mmio_base + offset);
}

static inline uint16_t io_read16(const io_region_t *io, uintptr_t offset){
	if(io->type == IO_TYPE_PIO){
		return port_read16(io->pio_base + offset);
	}

	return mmio_read16(io->mmio_base + offset);
}

static inline uint32_t io_read32(const io_region_t *io, uintptr_t offset){
	if(io->type == IO_TYPE_PIO){
		return port_read32(io->pio_base + offset);
	}

	return mmio_read32(io->mmio_base + offset);
}

static inline void io_write8(const io_region_t *io, uintptr_t offset, uint8_t val){
	if(io->type == IO_TYPE_PIO){
		port_write8(io->pio_base + offset, val);
	}

	mmio_write8(io->mmio_base + offset, val);
}

static inline void io_write16(const io_region_t *io, uintptr_t offset, uint16_t val){
	if(io->type == IO_TYPE_PIO){
		port_write16(io->pio_base + offset, val);
	}

	mmio_write16(io->mmio_base + offset, val);
}

static inline void io_write32(const io_region_t *io, uintptr_t offset, uint32_t val){
	if(io->type == IO_TYPE_PIO){
		port_write32(io->pio_base + offset, val);
	}

	mmio_write32(io->mmio_base + offset, val);
}

#endif
