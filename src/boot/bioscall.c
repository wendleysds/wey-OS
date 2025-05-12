#include "boot.h"

extern void bios_int10h(const struct biosreg* reg);
extern void bios_int13h(const struct biosreg* reg);
extern void bios_int15h(const struct biosreg* reg);
extern void bios_int16h(const struct biosreg* reg);

/*
 * bios interrupt call
 */
void bios_intcall(int int_no, const struct biosreg *inReg, struct biosreg *outReg){
	struct biosreg temp; // register that will be overwrited in the bios_intxxh

	// Copy from inReg
	temp.ax = inReg->ax;
	temp.bx = inReg->bx;
	temp.cx = inReg->cx;
	temp.dx = inReg->dx;
	temp.di = inReg->di;
	temp.es = inReg->es;

	switch (int_no) {
		case 0x10: 
			bios_int10h(&temp);
			break;
		case 0x13: 
			bios_int13h(&temp);
			break;
		case 0x15: 
			bios_int15h(&temp);
			break;
		case 0x16: 
			bios_int16h(&temp);
			break;
	}

	// Save result in outReg
	outReg->ax = temp.ax;
	outReg->bx = temp.bx;
	outReg->cx = temp.cx;
	outReg->dx = temp.dx;
	outReg->di = temp.di;
	outReg->es = temp.es;
}
