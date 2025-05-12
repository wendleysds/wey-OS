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
	struct biosreg oreg;
	struct biosreg ireg = { .ax=0x4F00, .ah=0x4F, .es=SEG((uint32_t)&infoBlock), .di=OFF((uint32_t)&infoBlock) };
	
	bios_intcall(0x10, &ireg, &oreg);

	if(oreg.ax == 0x004f){
		bios_printf("\nVesa Detected, Displaying info... \r\n\n");

		bios_printf("Signature: %s\r\n", infoBlock.VbeSignature);
		bios_printf("Version: 0x%x\r\n", infoBlock.VbeVersion);
		bios_printf("Total Memory: %d\r\n", infoBlock.TotalMemory);
		
		bios_printf("\nSupported Modes\r\n\n");

		struct VbeInfoMode infoMode;
		uint16_t *modes = (uint16_t*)((infoBlock.VideoModePtr[1] << 4) + infoBlock.VideoModePtr[0]);

		ireg.ax = 0x4F01;
		ireg.es = SEG((uint32_t)&infoMode);
		ireg.di = OFF((uint32_t)&infoMode);

		int i, j = 0;
		for(i = 0; modes[i] != 0xFFFF; i++){
			ireg.cx = modes[i];
			bios_intcall(0x10, &ireg, &oreg);
			if(oreg.ax != 0x004F) continue;

			bios_printf("{0x%x - %dx%d %d}",
					modes[i], 
					infoMode.width, 
					infoMode.height, 
					infoMode.bpp
			);

			if(j % 3 == 0)
				bios_printf("\r\n");

			j++;
		}
	}
	else{
		bios_printf("%s\r\n", "Vesa Not Detected");
	}

	while(1){
		__asm__ __volatile__ ("hlt");
	}

	go_to_protect_mode();
}

