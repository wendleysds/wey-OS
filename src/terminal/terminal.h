#ifndef _TERMINAL_H
#define _TERMINAL_H

#define TERMINAL_DEFAULT_COLOR 0x0F

void terminal_clear();
void terminal_write(const char* message, unsigned char color);

#endif
