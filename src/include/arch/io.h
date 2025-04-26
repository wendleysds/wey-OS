#ifndef _IO_H
#define _IO_H
#include <stdint.h>

void outb(uint16_t port, uint8_t val);
uint8_t insb(uint8_t port);

void outw(uint16_t port, uint16_t val);
uint16_t insw(uint16_t port);

#endif
