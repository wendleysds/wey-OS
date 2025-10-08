#include <boot/bios.h>
#include <boot/video.h>
#include <def/compile.h>
#include <stdint.h>

/*
 * Main module for the real-mode kernel code
 */

extern void go_to_protect_mode();

void initregs(struct biosreg *reg){
	memset(reg, 0, sizeof(*reg));
	reg->eflags |= (((1UL)) << (0));
	reg->ds = ds();
	reg->es = ds();
	reg->fs = fs();
	reg->gs = gs();
}

// TODO: Implement missing
__section(".text.entry")
void main(){

	// Check CPU

	// Check Memory

	struct biosreg ireg;
	struct biosreg oreg;
	initregs(&ireg);
	initregs(&oreg);

	ireg.bx = 0x0007;
	ireg.cx = 0x0001;
	ireg.ah = 0x0E;
	ireg.al = 'A';

	intcall(0x10, &ireg, 0x0);

	while(1){
		__asm__ volatile ("hlt");
	}

	setup_video();

	go_to_protect_mode();
}

