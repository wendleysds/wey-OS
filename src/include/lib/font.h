#ifndef _FONT_H
#define _FONT_H

#include <stdint.h>

struct font_info{
	uint16_t width;
	uint16_t heigth;
	void* data;
}__attribute__((packed));

#endif
