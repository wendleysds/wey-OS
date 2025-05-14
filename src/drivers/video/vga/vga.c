#include <drivers/video/vga.h>
#include <drivers/terminal.h>
#include <io.h>

#define _CURSOR_START 6
#define _CURSOR_END 7

/*
 * VGA Text Mode controller module
 */

void vga_set_cursor_position(struct VideoStructPtr* videoInfo, uint8_t x, uint8_t y){
	uint16_t pos = y * videoInfo->width + x;

  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)(pos & 0xFF));

  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

// only for text mode
void vga_putchar(struct VideoStructPtr* videoInfo, const char c, unsigned char color, uint8_t x, uint8_t y){
	volatile char* videoMemory = (volatile char*)videoInfo->framebuffer_physical;
	int index = (y * videoInfo->width + x) * 2;
	videoMemory[index] = c;
  videoMemory[index + 1] = color;
}

void vga_putpixel(struct VideoStructPtr* videoInfo, unsigned char color, uint8_t x, uint8_t y){
	volatile char* videoMemory = (volatile char*)videoInfo->framebuffer_physical;
	int index = (y * videoInfo->width + x) * 2;
  videoMemory[index] = color;
}


void vga_cursor_disable(){
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void vga_cursor_enable(uint8_t cursorStart, uint8_t cursorEnd){
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursorStart);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursorEnd);
}

