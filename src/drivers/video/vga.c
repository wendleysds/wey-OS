#include <lib/font.h>
#include <lib/string.h>
#include <wey/terminal_struct.h>
#include <wey/terminal.h>
#include <wey/mmu.h>
#include <uapi/headers.h>
#include <def/init.h>
#include <def/err.h>
#include <def/config.h>
#include <drivers/vga.h>

#define CURSOR_START 0x0
#define CUSOR_END 0xD

static unsigned long vga_vram_base;
static unsigned long vga_vram_end;
static int vga_vram_size;
static unsigned long vga_num_columns, vga_num_lines;
static int vga_scan_lines, vga_font_height = 16;

static uint16_t vga_reg_port, vga_reg_val;
static uint8_t vga_video_type;
static uint8_t vga_cursor_enabled = 1;
static uint8_t vga_support_color = 0;

int vga_setup(struct video_info* video_info){
	if (!video_info)
		return INVALID_ARG;

	if (video_info->width == 0 || video_info->height == 0)
		return INVALID_ARG;

	// VGA16 modes not supported
	if (video_info->mode == 0x0D || video_info->mode == 0x0E ||
		video_info->mode == 0x10 || video_info->mode == 0x12 ||
		video_info->mode == 0x6A)
		return INVALID_ARG;

	vga_num_lines = video_info->height;
	vga_num_columns =video_info->width;

	if (video_info->mode == 7) {
		// Mono
		vga_vram_base = 0xB0000;
		vga_reg_port = VGA_CRT_IM;
		vga_reg_val = VGA_CRT_DM;
		vga_support_color = 0;

		if ((video_info->mode & 0xff) != 0x10) {
			vga_video_type = VIDEO_TYPE_EGAM;
			vga_vram_size = 0x8000;
		} else {
			vga_video_type = VIDEO_TYPE_MDA;
			vga_vram_size = 0x2000;
			vga_font_height = 14;
		}
	} else {
		// Color
		vga_vram_base = 0xB8000;
		vga_reg_port = VGA_CRT_IC;
		vga_reg_val = VGA_CRT_DC;
		vga_support_color = 1;

		if ((video_info->ega_bx & 0xff) != 0x10) {
			vga_vram_size = 0x8000;
			vga_video_type = video_info->isVGA ? 
				VIDEO_TYPE_VGAC : VIDEO_TYPE_EGAC;
		} else {
			vga_video_type = VIDEO_TYPE_CGA;
			vga_vram_size = 0x2000;
			vga_font_height = 8;
		}
	}

	vga_vram_end = vga_vram_base + vga_vram_size;

	if (vga_video_type == VIDEO_TYPE_EGAC ||
		vga_video_type == VIDEO_TYPE_VGAC ||
		vga_video_type == VIDEO_TYPE_EGAM)
		vga_scan_lines = vga_font_height * vga_num_lines;

	return SUCCESS;
}

static void vga_init(struct vt_data *vt){
	vt->font.heigth = vt->vt_cell_height = vga_font_height;
	vt->vt_scan_lines = vga_scan_lines;
	vt->vt_cols = vga_num_columns;
	vt->vt_rows = vga_num_lines;
}

static void vga_putc(struct vt_data *vt, uint16_t ca, unsigned int x, unsigned int y)
{
	unsigned long offset = (y * vga_num_columns + x) * 2;
	unsigned long addr = vga_vram_base + offset;
	
	if (addr >= vga_vram_end)
		return;
	
	*(uint16_t *)addr = (uint16_t)ca;
}

static void vga_clean(struct vt_data *vt){
	
}

static void vga_cursor(struct vt_data *vt, uint8_t enabled){
	if (!vt->vt_is_text_mode)
		return;

	if (enabled) {
		unsigned long pos = vt->state.y * vga_num_columns + vt->state.x;
		outb(vga_reg_port, VGA_CRTC_CURSOR_LO);
		outb(vga_reg_val, (uint8_t)(pos & 0xFF));
		outb(vga_reg_port, VGA_CRTC_CURSOR_HI);
		outb(vga_reg_val, (uint8_t)((pos >> 8) & 0xFF));
	}

	if (enabled == vga_cursor_enabled)
		return;

	outb(vga_reg_port, VGA_CRTC_CURSOR_START);
	if (enabled) {
		outb(vga_reg_val, (inb(vga_reg_val) & 0xC0) | CURSOR_START);
		outb(vga_reg_port, VGA_CRTC_CURSOR_END);
		outb(vga_reg_val, (inb(vga_reg_val) & 0xE0) | CUSOR_END);
	} else {
		outb(vga_reg_val, 0x20);
	}

	vga_cursor_enabled = enabled;
}

static void vga_exit_early(){
	mmu_mmap(
		(void*)vga_vram_base,
		(void*)0x0,
		vga_vram_size,
		(MEM_DEVICE | MEM_KERNEL | MEM_READ | MEM_WRITE)
	);

	vga_vram_base = 0x0;
	vga_vram_end = vga_vram_base + vga_vram_size;
}

static uint8_t vga_build_attr(struct vt_data* vt, uint8_t color, enum vt_intensity intensity){
	uint8_t fg = VT_FG(color);
	uint8_t bg = VT_BG(color);
	uint8_t attr;

	if (intensity == TERMINAL_INTENSITY_BOLD) {
		fg |= 0x8;
	} else {
		fg &= 0x7;
	}

	attr = (bg << 4) | (fg & 0x0F);

	return attr;
}

static void vga_scroll(struct vt_data *vt, unsigned int top, unsigned int bottom, terminal_scroll_t scroll_dir, unsigned int lines){
	volatile uint16_t *buf = (volatile uint16_t*)vga_vram_base;
    unsigned int cols = vt->vt_cols;
    unsigned int stride = cols;

    unsigned int region_rows = bottom - top;
    unsigned int copy_rows = region_rows - lines;

    if (scroll_dir == TERMINAL_SCROLL_UP) {
        memmove(
            (void*)&buf[top * stride],
            (void*)&buf[(top + lines) * stride],
            copy_rows * stride * sizeof(uint16_t)
        );

        for (unsigned int row = bottom - lines; row < bottom; row++) {
            volatile uint16_t *line = &buf[row * stride];
            for (unsigned int col = 0; col < cols; col++)
                line[col] = vt->vt_erase_char;
        }

    } else {
        memmove(
            (void*)&buf[(top + lines) * stride],
            (void*)&buf[top * stride],
            copy_rows * stride * sizeof(uint16_t)
        );

        for (unsigned int row = top; row < top + lines; row++) {
            volatile uint16_t *line = &buf[row * stride];
            for (unsigned int col = 0; col < cols; col++)
                line[col] = vt->vt_erase_char;
        }
    }
}

const struct consw vgadrv_consw = {
	.init = vga_init,
	.setup = vga_setup,
	.putc = vga_putc,
	.clean = vga_clean,
	.cursor = vga_cursor,
	.scroll = vga_scroll,
	.build_attr = vga_build_attr,
	.exit_early = vga_exit_early,
};
