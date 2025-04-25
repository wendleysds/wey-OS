#ifndef _VGA_H
#define _VGA_H

#include <stdint.h>

void vga_putchar(const char c, unsigned char color);

void vga_set_cursor(uint8_t x, uint8_t y);
void vga_update_cursor(); 

void vga_cursor_disable();
void vga_cursor_enable();

#endif
