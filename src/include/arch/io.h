#ifndef _IO_H
#define _IO_H
#include <stdint.h>

void outb(uint16_t port, uint8_t val);
uint8_t insb(uint8_t port);

void outw(uint16_t port, uint16_t val);
uint16_t insw(uint16_t port);

static inline uint8_t inb(uint16_t port){
	uint8_t value;
	__asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

#endif
