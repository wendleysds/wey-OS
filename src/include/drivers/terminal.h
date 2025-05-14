#ifndef _TERMINAL_H
#define _TERMINAL_H
#include <stdint.h>
#include <stdarg.h>

#define TERMINAL_DEFAULT_COLOR 0x0F

struct Cursor{
	uint8_t x, y;
	uint8_t enabled;
}__attribute__ ((packed));

struct VideoStructPtr{
	uint16_t mode;

	uint16_t width;
	uint16_t height;
	uint16_t pitch;
	uint8_t bpp;

	uint32_t framebuffer_physical;	
	uint32_t framebuffer_virtual;
	//flags
	uint8_t isGraphical;
	uint8_t isVesa;
} __attribute__ ((packed));

void terminal_init();

void terminal_cursor_enable_SE(uint8_t cursor_start, uint8_t cursor_end);
void terminal_cursor_enable();
void terminal_cursor_disable();

void terminal_clear();
void terminal_vwrite(unsigned char color, const char *format, va_list args);
void terminal_write(unsigned char color, const char* format, ...);
void terminal_cwrite(unsigned char color, const char *format, ...);
void terminal_backspace();

#endif
