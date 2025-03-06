#ifndef _TERMINAL_H
#define _TERMINAL_H
#include <stdint.h>

#define TERMINAL_DEFAULT_COLOR 0x0F

void terminal_init();

void terminal_cursor_enable();
void terminal_cursor_enable(uint8_t cursor_start, uint8_t cursor_end);
void terminal_cursor_disable();

void terminal_clear();
void terminal_write(const char* message, unsigned char color);
void terminal_backspace();

#endif
