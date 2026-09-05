#ifndef _IO_PORTS_H
#define _IO_PORTS_H

#include <stdint.h>

// New API: Use port_read8, port_read16, port_read32, port_write8, port_write16, port_write32
#ifdef PORT_NEW_API

static inline uint8_t port_read8 (uint16_t port){
	uint8_t value;

	__asm__ volatile ("inb %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline uint16_t port_read16 (uint16_t port){
	unsigned short value;

	__asm__ volatile ("inw %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline uint32_t port_read32 (uint16_t port){
	uint32_t value;

	__asm__ volatile ("inl %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline void port_write8 (uint16_t port, uint8_t val){
	__asm__ volatile ("outb %b0,%w1": :"a" (val), "Nd" (port));
}

static inline void port_write16 (uint16_t port, uint16_t val){
	__asm__ volatile ("outw %w0,%w1": :"a" (val), "Nd" (port));
}

static inline void port_write32 (uint16_t port, uint32_t val){
	__asm__ volatile ("outl %0,%w1": :"a" (val), "Nd" (port));
}

// Pause variants

static inline uint8_t port_read8_p (uint16_t port){
	uint8_t value;

	__asm__ volatile ("inb %w1,%0\noutb %%al,$0x80":"=a" (value):"Nd" (port));
	return value;
}

static inline uint16_t port_read16_p (uint16_t port){
	uint16_t value;

	__asm__ volatile ("inw %w1,%0\noutb %%al,$0x80":"=a" (value):"Nd" (port));
	return value;
}

static inline uint32_t port_read32_p (uint16_t port){
	uint32_t value;
	__asm__ volatile ("inl %w1,%0\noutb %%al,$0x80":"=a" (value):"Nd" (port));
	return value;
}

static inline void port_write8_p (uint16_t port, uint8_t val){
	__asm__ volatile ("outb %b0,%w1\noutb %%al,$0x80": :"a" (val), "Nd" (port));
}

static inline void port_write16_p (uint16_t port, uint16_t val){
	__asm__ volatile ("outw %w0,%w1\noutb %%al,$0x80": :"a" (val), "Nd" (port));
}

static inline void port_write32_p (uint16_t port, uint32_t val){
	__asm__ volatile ("outl %0,%w1\noutb %%al,$0x80": :"a" (val), "Nd" (port));
}

// REP instructions

static inline void port_readsb (uint16_t port, void *addr, uint32_t count){
	__asm__ volatile ("cld ; rep ; insb":"=D" (addr), "=c" (count) 
		:"d" (port), "0" (addr), "1" (count));
}

static inline void port_readsw (uint16_t port, void *addr, uint32_t count) {
	__asm__ volatile ("cld ; rep ; insw":"=D" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

static inline void port_readsl (uint16_t port, void *addr, uint32_t count){
	__asm__ volatile ("cld ; rep ; insl":"=D" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

static inline void port_writesb (uint16_t port, const void *addr, uint32_t count){
	__asm__ volatile ("cld ; rep ; outsb":"=S" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

static inline void port_writesw (uint16_t port, const void *addr, uint32_t count){
	__asm__ volatile ("cld ; rep ; outsw":"=S" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

static inline void port_writesl (uint16_t port, const void *addr, uint32_t count){
	__asm__ volatile ("cld ; rep ; outsl":"=S" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

#else // Old API: Compatibility with old code (will be removed in the future)

static inline uint8_t inb (uint16_t port){
	uint8_t value;

	__asm__ volatile ("inb %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline uint16_t inw (uint16_t port){
	unsigned short value;

	__asm__ volatile ("inw %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline uint32_t inl (uint16_t port){
	uint32_t value;

	__asm__ volatile ("inl %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline void outb (uint16_t port, uint8_t val){
	__asm__ volatile ("outb %b0,%w1": :"a" (val), "Nd" (port));
}


static inline void outw (uint16_t port, uint16_t val){
	__asm__ volatile ("outw %w0,%w1": :"a" (val), "Nd" (port));
}

static inline void outl (uint16_t port, uint32_t val){
	__asm__ volatile ("outl %0,%w1": :"a" (val), "Nd" (port));
}

// Pause variants

static inline uint8_t inb_p (uint16_t port){
	uint8_t value;

	__asm__ volatile ("inb %w1,%0\noutb %%al,$0x80":"=a" (value):"Nd" (port));
	return value;
}

static inline uint16_t inw_p (uint16_t port){
	uint16_t value;

	__asm__ volatile ("inw %w1,%0\noutb %%al,$0x80":"=a" (value):"Nd" (port));
	return value;
}

static inline uint32_t inl_p (uint16_t port){
	uint32_t value;
	__asm__ volatile ("inl %w1,%0\noutb %%al,$0x80":"=a" (value):"Nd" (port));
	return value;
}

static inline void outb_p (uint16_t port, uint8_t val){
	__asm__ volatile ("outb %b0,%w1\noutb %%al,$0x80": :"a" (val), "Nd" (port));
}

static inline void outw_p (uint16_t port, uint16_t val){
	__asm__ volatile ("outw %w0,%w1\noutb %%al,$0x80": :"a" (val), "Nd" (port));
}

static inline void outl_p (uint16_t port, uint32_t val){
	__asm__ volatile ("outl %0,%w1\noutb %%al,$0x80": :"a" (val), "Nd" (port));
}

// REP instructions

static inline void insb (uint16_t port, void *addr, uint32_t count){
	__asm__ volatile ("cld ; rep ; insb":"=D" (addr), "=c" (count) 
		:"d" (port), "0" (addr), "1" (count));
}

static inline void insw (uint16_t port, void *addr, uint32_t count) {
	__asm__ volatile ("cld ; rep ; insw":"=D" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

static inline void insl (uint16_t port, void *addr, uint32_t count){
	__asm__ volatile ("cld ; rep ; insl":"=D" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

static inline void outsb (uint16_t port, const void *addr, uint32_t count){
	__asm__ volatile ("cld ; rep ; outsb":"=S" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

static inline void outsw (uint16_t port, const void *addr, uint32_t count){
	__asm__ volatile ("cld ; rep ; outsw":"=S" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

static inline void outsl (uint16_t port, const void *addr, uint32_t count){
	__asm__ volatile ("cld ; rep ; outsl":"=S" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

#endif

#endif
