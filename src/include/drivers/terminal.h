#ifndef _TERMINAL_H
#define _TERMINAL_H
#include <stdint.h>
#include <stdarg.h>

#define TERMINAL_DEFAULT_COLOR 0x0F

void terminal_init();

void terminal_cursor_enable_SE(uint8_t cursor_start, uint8_t cursor_end);
void terminal_cursor_enable();
void terminal_cursor_disable();

void terminal_clear();
void terminal_vwrite(unsigned char color, const char *format, va_list args);
void terminal_write(unsigned char color, const char* format, ...);
void terminal_backspace();

#endif
