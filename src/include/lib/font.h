#ifndef _FONT_H
#define _FONT_H

#include <stdint.h>

struct FontInfo{
	uint8_t width;
	uint8_t heigth;
	const unsigned char* ptr;
}__attribute__((packed));

extern unsigned char font8x16[][16];

#endif
