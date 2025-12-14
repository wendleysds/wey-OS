#include "boot.h"

static int kbd_pending()
{
	struct biosregs ireg, oreg;

	initregs(&ireg);
	ireg.ah = 0x01;
	intcall(0x16, &ireg, &oreg);

	return !(oreg.eflags & X86_EFLAGS_ZF);
}

int kbd_getchar(){
	struct biosregs ireg, oreg;
	initregs(&ireg);
	intcall(0x16, &ireg, &oreg);

	return oreg.al;
}

int kdb_read(char* restrict buffer, int length){
	for(int i = 0; i < length; i++)
		buffer[i] = kbd_getchar();
}

void kbd_flush()
{
	for (;;) {
		if (!kbd_pending())
			break;
		getchar();
	}
}