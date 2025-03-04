#include "terminal.h"
#include <stdint.h>

#define VGA_MEMORY (volatile char*)0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGTH 25

struct Cursor{
	uint8_t x, y;
};

static struct Cursor cursor;
static volatile char* videoMemory = VGA_MEMORY;

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void update_cursor() {
    uint16_t pos = cursor.y * VGA_WIDTH + cursor.x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
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
  } else {
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
  update_cursor();
}

void terminal_write(const char* chars, unsigned char color) {
	for(int i = 0; chars[i] != '\0'; i++){
		terminal_putchar(chars[i], color);
	}
}



