#include "boot.h"
#include "vesa.h"
#include <drivers/terminal.h>
#include <stddef.h>
#include <stdint.h>

#define VIDEO_STRUCT_ADDR 0x8000 // VideoStruct save point

#define DEFAULT_MODE 0x13 // 320x200 16

#define DESIRED_RESOLUTION       800, 600, 16  //W, H, BPP
#define MOST_COMPATIBILITIE_MODE 640, 480, 16  //W, H, BPP

// Try to find the mode with the desired resolution and bpp.
// If it doesn't find it, it returns DEFAULT_MODE and fills outinfo with its data.
static uint16_t _find_mode(uint16_t* modes, struct VbeInfoMode* outinfo, uint16_t x, uint16_t y, uint8_t bpp){
	struct biosregs ireg, oreg;
	initregs(&ireg);
	ireg.ax = 0x4F01;
	ireg.ah = 0x4F;
	ireg.es = SEG(outinfo);
	ireg.di = OFF(outinfo);

	int i;
	for(i = 0; modes[i] != 0xFFFF; i++){
		ireg.cx = modes[i];
		intcall(0x10, &ireg, &oreg);

		if(oreg.ax != 0x004F) continue;

		// Check if this is a graphics mode with linear frame buffer support
		if((outinfo->attributes & 0x90) != 0x90){
			continue;
		}

		// Check if this is a packed pixel or direct color mode
		if(outinfo->memory_model != 4 && outinfo->memory_model != 6){
			continue;
		}

		if(x == outinfo->width && y == outinfo->height && bpp == outinfo->bpp){
			return modes[i];
		}
	}

	ireg.cx = DEFAULT_MODE;
	intcall(0x10, &ireg, &oreg);
	return DEFAULT_MODE;
}

void setup_video(){
	struct VbeInfoBlock infoBlock;
	struct VbeInfoMode info;

	// Default Mode
	struct VideoStruct videoStruct = { 
		.mode=0x3,
		.width=80, .height=25, .bpp=4, .pitch=0x1,

		.framebuffer_physical = 0xB8000,
		.framebuffer_virtual = 0x0,

		.isGraphical = 0,
		.isVesa = 0
	};

	struct biosregs ireg, oreg;
	initregs(&ireg);
	ireg.ax = 0x4F00;
	ireg.ah = 0x4F;
	ireg.es = SEG(&infoBlock);
	ireg.di = OFF(&infoBlock);

	intcall(0x10, &ireg, &oreg);

	if(oreg.ax == 0x004f){
		uint16_t *modes = (uint16_t*)((infoBlock.VideoModePtr[1] << 4) + infoBlock.VideoModePtr[0]);

		uint16_t findedMode = _find_mode(modes, &info, DESIRED_RESOLUTION);

		// If the mode is the default one, try the most compatibilitie mode
		if(findedMode == DEFAULT_MODE)
			findedMode = _find_mode(modes, &info, MOST_COMPATIBILITIE_MODE);

		ireg.ax = 0x4F02;
		ireg.bx = findedMode | (1 << 14);
		ireg.es = 0;
		ireg.di = 0;

		intcall(0x10, &ireg, 0x0);

		videoStruct.mode = findedMode;
		videoStruct.width = info.width;
		videoStruct.height = info.height;
		videoStruct.pitch = info.pitch;
		videoStruct.bpp = info.bpp;

		videoStruct.framebuffer_physical = info.framebuffer,
		videoStruct.framebuffer_virtual = 0x0,

		videoStruct.isGraphical = !(findedMode <= 0x3 || findedMode == 0x7);
		videoStruct.isVesa = !(findedMode >= 0 && findedMode <= 0x16);
	}
	else{
		bios_printf("%s\r\n", "Vesa Not Detected");
	}

	memcpy(
		(uint8_t*)VIDEO_STRUCT_ADDR, 
		&videoStruct, 
		sizeof(struct VideoStructPtr)
	);
}

