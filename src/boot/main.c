#include "boot.h"
#include "video.h"
#include <stdint.h>

/*
 * Main module for the real-mode kernel code
 */

extern void go_to_protect_mode();

// TODO: Implement missing
void main(){

	// Check CPU

	// Check Memory

	struct VbeInfoBlock infoBlock;
	struct biosreg reg = { .ax=0x4F00, .es=SEG((uint32_t)&infoBlock), .di=OFF((uint32_t)&infoBlock) };

	bios_int10h(&reg);

	if(reg.ax == 0x004f){
		bios_printf("\nVesa Detected, Displaying video... \r\n\n");

		bios_printf("Signature: %s\r\n", infoBlock.VbeSignature);
		bios_printf("Version: 0x%x\r\n", infoBlock.VbeVersion);
		bios_printf("Total Memory: %d\r\n", infoBlock.TotalMemory);

		bios_printf("\nSupported Modes\r\n\n");

		struct VbeInfoMode infoMode;
		uint16_t *modes = (uint16_t*)((infoBlock.VideoModePtr[1] << 4) + infoBlock.VideoModePtr[0]);

		int i;
		for(i = 0; modes[i] != 0xFFFF; i++){
			reg.ax = 0x4F01;
			reg.cx = modes[i];
			reg.es = SEG((uint32_t)&infoMode);
			reg.di = OFF((uint32_t)&infoMode);
			bios_int10h(&reg);
			if(reg.ax != 0x004F) continue;

			bios_printf("%dx%d\r\n", infoMode.width, infoMode.height);
		}

		bios_printf("\nTotal Modes: %d\r\n", i);
	}
	else{
		bios_printf("%s\r\n", "Vesa Not Detected");
	}

	while(1){
		__asm__ __volatile__ ("hlt");
	}

	go_to_protect_mode();
}

