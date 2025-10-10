#include "boot.h"

void initregs(struct biosregs *reg){
	memset(reg, 0, sizeof(*reg));
	reg->eflags |= (((1UL)) << (0));
	reg->ds = ds();
	reg->es = ds();
	reg->fs = fs();
	reg->gs = gs();
}