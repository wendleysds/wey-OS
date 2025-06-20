#ifndef _IO_PORTS_H
#define _IO_PORTS_H
#include <stdint.h>

static inline uint8_t inb (uint16_t port){
	uint8_t value;

	__asm__ volatile ("inb %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline uint8_t inb_p (uint16_t port){
	uint8_t value;

	__asm__ volatile ("inb %w1,%0\noutb %%al,$0x80":"=a" (value):"Nd" (port));
	return value;
}

static inline uint16_t inw (uint16_t port){
	unsigned short value;

	__asm__ volatile ("inw %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline uint16_t inw_p (uint16_t port){
	uint16_t value;

	__asm__ volatile ("inw %w1,%0\noutb %%al,$0x80":"=a" (value):"Nd" (port));
	return value;
}

static inline uint32_t inl (uint16_t port){
	uint32_t value;

	__asm__ volatile ("inl %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline uint32_t inl_p (uint16_t port){
	uint32_t value;
	__asm__ volatile ("inl %w1,%0\noutb %%al,$0x80":"=a" (value):"Nd" (port));
	return value;
}

static inline void outb (uint16_t port, uint8_t val){
	__asm__ volatile ("outb %b0,%w1": :"a" (val), "Nd" (port));
}

static inline void outb_p (uint16_t port, uint8_t val){
	__asm__ volatile ("outb %b0,%w1\noutb %%al,$0x80": :"a" (val), "Nd" (port));
}

static inline void outw (uint16_t port, uint16_t val){
	__asm__ volatile ("outw %w0,%w1": :"a" (val), "Nd" (port));
}

static inline void outw_p (uint16_t port, uint16_t val){
	__asm__ volatile ("outw %w0,%w1\noutb %%al,$0x80": :"a" (val), "Nd" (port));
}

static inline void outl (uint16_t port, uint32_t val){
	__asm__ volatile ("outl %0,%w1": :"a" (val), "Nd" (port));
}

static inline void outl_p (uint16_t port, uint32_t val){
	__asm__ volatile ("outl %0,%w1\noutb %%al,$0x80": :"a" (val), "Nd" (port));
}

static inline void insb (uint16_t port, void *addr, uint64_t count){
	__asm__ volatile ("cld ; rep ; insb":"=D" (addr), "=c" (count) 
		:"d" (port), "0" (addr), "1" (count));
}

static inline void insw (uint16_t port, void *addr, uint32_t count) {
	__asm__ volatile ("cld ; rep ; insw":"=D" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

static inline void insl (uint16_t port, void *addr, uint64_t count){
	__asm__ volatile ("cld ; rep ; insl":"=D" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

static inline void outsb (uint16_t port, const void *addr, uint64_t count){
	__asm__ volatile ("cld ; rep ; outsb":"=S" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

static inline void outsw (uint16_t port, const void *addr, uint32_t count){
	__asm__ volatile ("cld ; rep ; outsw":"=S" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

static inline void outsl (uint16_t port, const void *addr, uint64_t count){
	__asm__ volatile ("cld ; rep ; outsl":"=S" (addr), "=c" (count)
		:"d" (port), "0" (addr), "1" (count));
}

#endif
