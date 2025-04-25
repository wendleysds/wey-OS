#include <drivers/terminal.h>
#include <lib/utils.h>
#include <arch/io.h>
#include <stdint.h>
#include <stdarg.h>

#define VGA_MEMORY (volatile char*)0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGTH 25

#define DEFAULT_CURSOR_START 6
#define DEFAULT_CURSOR_END 7

#define DEFAULT_TAB_DISTANCE 4

struct Cursor{
	uint8_t x, y;
	uint8_t enabled;
};

static struct Cursor cursor;
static volatile char* videoMemory = VGA_MEMORY;

void terminal_init(){
	cursor.y = 0;
	cursor.x = 0;
	cursor.enabled = 1;
}

void update_cursor() {
	if(!cursor.enabled)
		return;

    uint16_t pos = cursor.y * VGA_WIDTH + cursor.x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_cursor_enable(){
	terminal_cursor_enable_SE(DEFAULT_CURSOR_START, DEFAULT_CURSOR_END);
}

void terminal_cursor_enable_SE(uint8_t cursor_start, uint8_t cursor_end){
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
	cursor.enabled = 1;
	update_cursor();
}

void terminal_cursor_disable(){
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
	cursor.enabled = 0;
}

void scroll_terminal(){
	if(cursor.y >= VGA_HEIGTH){
		for(int y = 1; y < VGA_HEIGTH; y++){
			for(int x = 0; x < VGA_WIDTH; x++){
				int from = (y * VGA_WIDTH + x) * 2;
				int to = ((y - 1) * VGA_WIDTH + x) * 2;

				videoMemory[to] = videoMemory[from]; // Copy char
				videoMemory[to + 1] = videoMemory[from + 1]; // Copy color
			}
		}

		// Clear last line
		for(int x = 0; x < VGA_WIDTH; x++){
			int index = ((VGA_HEIGTH - 1) * VGA_WIDTH + x) * 2;
			videoMemory[index] = ' ';
			videoMemory[index + 1] = 0x0F;
		}

		cursor.y = VGA_HEIGTH - 1; // Adjust to last visible line
	}
}

void terminal_clear() {
	for(int i = 0; i < VGA_WIDTH * VGA_HEIGTH; i++){
		videoMemory[i * 2] = ' ';
		videoMemory[i * 2 + 1] = 0x0F;
	}
	cursor.x = 0;
	cursor.y = 0;
	update_cursor();
}

void terminal_putchar(char c, unsigned char color) {
	if (c == '\n') {
      cursor.y += 1;
      cursor.x = 0;
	}
	else if(c == '\r'){
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
	
	scroll_terminal();
}

void terminal_backspace(){
	if(cursor.x == 0 && cursor.y != 0)
		return;

	if(cursor.x == 0){
		cursor.y -= 1;
		cursor.x = VGA_WIDTH;
	}
	
	cursor.x -= 1;
	terminal_putchar(' ', TERMINAL_DEFAULT_COLOR);
	cursor.x -= 1;
	update_cursor();
}

void terminal_write(unsigned char color, const char *format, ...){
	va_list args;
  va_start(args, format);

  char buffer[32];
  for (const char *ptr = format; *ptr; ptr++) {
		if (*ptr == '%') {
			ptr++;
			switch (*ptr) {
				case 'c':
					terminal_putchar(va_arg(args, int), color);
					break;
				case 's':
					for (char *s = va_arg(args, char*); *s; s++)
						terminal_putchar(*s, color);
					break;
				case 'd':
					itoa(va_arg(args, int), buffer, 10);
					for (char *s = buffer; *s; s++)
						terminal_putchar(*s, color);
					break;
				case 'x':
					itoa(va_arg(args, int), buffer, 16);
					for (char *s = buffer; *s; s++)
						terminal_putchar(*s, color);
					break;
				default:
					terminal_putchar('%', color);
					terminal_putchar(*ptr, color);
					break;
			}
		} else {
			terminal_putchar(*ptr, color);
		}
	}
	va_end(args);

	update_cursor();
}

