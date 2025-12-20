#include "boot.h"
#include "vesa.h"

struct vga_mode{
	uint8_t mode;
	uint16_t width;
	uint16_t height;
	uint16_t bpp;
	uint32_t vrambase;
};

static uint8_t vbe_supported = 0;
static struct VbeInfoBlock infoBlock;
static struct VbeInfoMode infoMode;

static const struct vga_mode vga_modes[] = {
	{0x00, 40, 25, 16, 0xB8000},
	{0x01, 80, 25, 0, 0xB0000},
	{0x02, 80, 25, 16, 0xB8000},
	{0x03, 80, 25, 0, 0xB8000},
	{0x04, 320, 200, 4, 0xA0000},
	{0x05, 320, 200, 4, 0xA0000},
	{0x06, 640, 200, 2, 0xA0000},
	{0x07, 80, 25, 0, 0xB0000},
	{0x0D, 320, 200, 16, 0xA0000},
	{0x0E, 640, 200, 16, 0xA0000},
	{0x0F, 640, 350, 0, 0xA0000},
	{0x10, 640, 350, 16, 0xA0000},
	{0x11, 640, 480, 2, 0xA0000},
	{0x12, 640, 480, 16, 0xA0000},
	{0x13, 320, 200, 256, 0xA0000},
};

struct vga_mode* _get_mode(uint8_t mode){
    mode &= 0x7F;

    for(size_t i = 0; i < ARRAY_SIZE(vga_modes); i++){
        if(vga_modes[i].mode == mode)
            return (struct vga_mode*)&vga_modes[i];
    }

    return NULL;
}


static inline int is_vga_text_mode(uint16_t mode) {
	mode &= 0x7F;
    return (mode > 0x00 && mode <= 0x03) || mode == 0x07;
}

static void save_current(){
	struct biosregs ireg, oreg;
	struct video_info* vi = &boot_header.video_info;

	// get current mode
	initregs(&ireg);
    ireg.ah = 0x0F;
    intcall(0x10, &ireg, &oreg);

    uint16_t vga_mode = oreg.ax & 0x7F;

	if(!vbe_supported) {
		goto no_vbe_mode;
	}

	// Try to get current VBE mode
	initregs(&ireg);
    ireg.ax = 0x4F03;
    intcall(0x10, &ireg, &oreg);

    uint16_t vbe_mode = 0xFFFF;
    int is_vbe = (oreg.ax == 0x004F);

	if (is_vbe) {
        vbe_mode = oreg.bx;

		// Get VBE mode info
        initregs(&ireg);
        ireg.ax = 0x4F01;
        ireg.cx = vbe_mode;
        ireg.di = (size_t)(&infoMode);
        intcall(0x10, &ireg, &oreg);
    }

	if (is_vbe && (infoMode.attributes & (1 << 4))) {
		// Graphic mode supported
        vi->mode = vbe_mode;

		vi->width = infoMode.width;
		vi->height = infoMode.height;
		vi->pitch = infoMode.pitch;
        vi->bpp = infoMode.bpp;

        vi->framebuffer = infoMode.framebuffer;
        vi->type = VIDEO_TYPE_VLFB;

        vi->red_mask = infoMode.red_mask;
        vi->red_position = infoMode.red_position;
        vi->green_mask = infoMode.green_mask;
        vi->green_position = infoMode.green_position;
        vi->blue_mask = infoMode.blue_mask;
        vi->blue_position = infoMode.blue_position;
		vi->reserved_mask = infoMode.reserved_mask;
		vi->reserved_position = infoMode.reserved_position;

		vi->attributes = infoMode.attributes;
		vi->direct_color_attributes = infoMode.direct_color_attributes;
		vi->pages = infoMode.image_pages;
		vi->capabilities = *((uint32_t*)infoBlock.Capabilities);

		vi->vesapm_off = OFF(infoMode.win_func_ptr);
		vi->vesapm_seg = SEG(infoMode.win_func_ptr);
		goto get_cursor_position;
    }
	
no_vbe_mode:
	// Fallback to VGA mode (text/graphics)
	vi->mode = vga_mode;
	vi->type = is_vga_text_mode(vga_mode) ? VIDEO_TYPE_EGAM : VIDEO_TYPE_VGAC;

	struct vga_mode* mode = _get_mode(vga_mode);
	if(mode){
		vi->width = mode->width;
		vi->height = mode->height;
		vi->bpp = mode->bpp;
		vi->pitch = mode->width / 8;
		vi->framebuffer = mode->vrambase;
	}

get_cursor_position:
	// Get cursor position
	initregs(&ireg);
	ireg.ah = 0x03;
	intcall(0x10, &ireg, &oreg);

	vi->orig_x = oreg.dl;
	vi->orig_y = oreg.dh;
}

static void check_vbe(){
	struct biosregs ireg, oreg;
	initregs(&ireg);

	memcpy(infoBlock.VbeSignature, "VBE2", 4);
	ireg.ax = 0x4F00;
	ireg.di = (size_t)(&infoBlock);
	intcall(0x10, &ireg, &oreg);

	vbe_supported = (oreg.ax == 0x004F);
}

static void query_mode(){

}

void setup_video(){
	check_vbe();



	save_current();
}

