#include <drivers/video/vga.h>
#include <io.h>

#define VGA_MEMORY (volatile char*)0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGTH 25

#define _CURSOR_START 6
#define _CURSOR_END 7

struct VGACursor{
	uint8_t x, y;
	uint8_t enabled;
};

static struct VGACursor cursor = {
	.x = 0, 
	.y = 0, 
	.enabled = 1
};

static volatile char* videoMemory = VGA_MEMORY;

void vga_update_cursor() {
	if(!cursor.enabled)
		return;

	uint16_t pos = cursor.y * VGA_WIDTH + cursor.x;

  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)(pos & 0xFF));

  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_putchar(const char c,  unsigned char color){
	if (c == '\n') {
		cursor.y += 1;
		cursor.x = 0;
	}
	else if (c == '\r'){
		cursor.x = 0;
	}
	else {
		int index = (cursor.y * VGA_WIDTH + cursor.x) * 2;
    videoMemory[index] = c;
    videoMemory[index + 1] = color;

    cursor.x += 1;
		if (cursor.x >= VGA_WIDTH) {
			cursor.y += 1;
			cursor.x = 0;
		}
	}
}

void vga_set_cursor(uint8_t x, uint8_t y){
	cursor.x = x;
	cursor.y = y;
}

void vga_cursor_disable(){
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
	cursor.enabled = 0;
}

void vga_cursor_enable(){
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | _CURSOR_START);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | _CURSOR_END);
	cursor.enabled = 1;
	vga_update_cursor();
}

