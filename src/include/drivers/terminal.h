#ifndef _TERMINAL_H
#define _TERMINAL_H
#include <stdint.h>
#include <stdarg.h>

#define TERMINAL_DEFAULT_COLOR 0xFFFFFFFF

struct Cursor{
	uint16_t x, y;
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

void terminal_clear();
void terminal_write(const char* format, ...);
void terminal_cwrite(uint32_t color, const char *format, ...);
void terminal_vwrite(uint32_t color, const char *format, va_list args);
void terminal_backspace();

#endif
