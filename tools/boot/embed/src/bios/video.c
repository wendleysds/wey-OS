#include <platform.h>
#include "bios.h"

uint8_t height, width, page;

void platform_init_video(){
	struct biosregs ireg, oreg;
	initregs(&ireg);

	ireg.ah = 0x0F;
	intcall(0x10, &ireg, &oreg);

	width = oreg.ah;
	page  = oreg.bh;

	if (width == 80) {
		height = 25;
	} else if (width == 40) {
		height = 25;
	} else if(width == 80 && oreg.al == 0x0) {
		height = 50;
	}else{
		height = 25;
	}
}

void platform_putchar(char c){
	struct biosregs r;
	initregs(&r);
	r.bx = 0x0007;
	r.cx = 0x0001;
	r.ah = 0x0E;
	r.al = c;
	intcall(0x10, &r, 0x0);
}

void platform_clear_screen(){
	struct biosregs ireg;
	initregs(&ireg);
	ireg.ah = 0x6;
	ireg.al = 0x0;
	ireg.bh = 0x7;

	ireg.dl = width - 1;
	ireg.dh = height - 1;

	intcall(0x10, &ireg, NULL);

	platform_set_cursor(0,0);
}

void platform_set_cursor(uint8_t x, uint8_t y){
	struct biosregs ireg;
	initregs(&ireg);

	ireg.ah = 0x02;
	ireg.bh = page;
	ireg.dh = y;
	ireg.dl = x;

	intcall(0x10, &ireg, NULL);
}

void platform_get_cursor(uint8_t* x, uint8_t* y){
	struct biosregs ireg;
	initregs(&ireg);

	ireg.ah = 0x03;
	ireg.bh = page;

	intcall(0x10, &ireg, NULL);

	if (x) *x = ireg.dl;
	if (y) *y = ireg.dh;
}

void platform_get_resolution(uint8_t* x, uint8_t* y){
	if (x) *x = width;
	if (y) *y = height;
}