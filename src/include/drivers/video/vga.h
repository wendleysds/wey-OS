#ifndef _VGA_H
#define _VGA_H

#include <drivers/terminal.h>
#include <stdint.h>

void vga_putchar(struct VideoStructPtr* videoInfo, const char c, unsigned char color, uint8_t x, uint8_t y);
void vga_putpixel(struct VideoStructPtr* videoInfo, unsigned char color, uint8_t x, uint8_t y);

void vga_set_cursor_position(struct VideoStructPtr* videoInfo, uint8_t x, uint8_t y);

void vga_cursor_disable();
void vga_cursor_enable(uint8_t cursorStart, uint8_t cursorEnd);

#endif
