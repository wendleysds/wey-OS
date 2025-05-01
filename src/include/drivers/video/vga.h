#ifndef _VGA_H
#define _VGA_H

#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGTH 25

void vga_putchar(const char c, unsigned char color, uint8_t x, uint8_t y);
void vga_set_cursor_position(uint8_t x, uint8_t y); 

void vga_cursor_disable();
void vga_cursor_enable(uint8_t cursorStart, uint8_t cursorEnd);

#endif
