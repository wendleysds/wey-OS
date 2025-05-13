#include "video.h"
#include "boot.h"
#include <stdint.h>

#define DEFAULT_MODE 0x13 // 320x200 16

//#define DESIRED_RESOLUTION 1024, 768, 16       //W, H, BPP
#define DESIRED_RESOLUTION 800, 600, 16        //W, H, BPP
#define MOST_COMPATIBILITIE_MODE 640, 480, 16  //W, H, BPP

// Try to find the mode with the desired resolution and bpp.
// If it doesn't find it, it returns DEFAULT_MODE and fills outinfo with its data.
static uint16_t findMode(uint16_t* modes, struct VbeInfoMode* outinfo, uint16_t x, uint16_t y, uint8_t bpp){
	struct biosreg oreg;
	struct biosreg ireg = { .ax=0x4F01, .ah=0x4F, .es=SEG((uint32_t)outinfo), .di=OFF((uint32_t)outinfo)};

	int i;
	for(i = 0; modes[i] != 0xFFFF; i++){
		ireg.cx = modes[i];
		bios_intcall(0x10, &ireg, &oreg);

		if(oreg.ax != 0x004F) continue;

		// Check if this is a graphics mode with linear frame buffer support
    if((outinfo->attributes & 0x90) != 0x90) continue;

    // Check if this is a packed pixel or direct color mode
    if(outinfo->memory_model != 4 && outinfo->memory_model != 6) continue;

		if(x == outinfo->width && y == outinfo->height && bpp == outinfo->bpp)
			return modes[i];
	}

	ireg.cx = DEFAULT_MODE;
	bios_intcall(0x10, &ireg, &oreg);
	return DEFAULT_MODE;
}

void setup_video(){
	struct VbeInfoBlock infoBlock;
	struct VbeInfoMode info;

	struct biosreg oreg;
	struct biosreg ireg = { .ax=0x4F00, .ah=0x4F, .es=SEG((uint32_t)&infoBlock), .di=OFF((uint32_t)&infoBlock) };
	bios_intcall(0x10, &ireg, &oreg);

	if(oreg.ax == 0x004f){
		uint16_t *modes = (uint16_t*)((infoBlock.VideoModePtr[1] << 4) + infoBlock.VideoModePtr[0]);

		uint16_t findedMode = findMode(modes, &info, DESIRED_RESOLUTION);

		// If the mode is the default one, try the most compatibilitie mode
		if(findedMode == DEFAULT_MODE)
			findedMode = findMode(modes, &info, MOST_COMPATIBILITIE_MODE);

		ireg.ax = 0x4F02;
		ireg.bx = findedMode | (1 << 14);
		ireg.es = 0;
		ireg.di = 0;

		bios_intcall(0x10, &ireg, 0x0);
	}
	else{
		bios_printf("%s\r\n", "Vesa Not Detected");
	}
}

