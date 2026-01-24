#ifndef _TERMINAL_STRUCT_H
#define _TERMINAL_STRUCT_H

enum vt_intensity {
	TERMINAL_INTENSITY_HALF_BRIGHT,
	TERMINAL_INTENSITY_NORMAL,
	TERMINAL_INTENSITY_BOLD,
	TERMINAL_INTENSITY_MASK = 0x3,
};

struct vt_state{
	unsigned int x, y;
	unsigned char color;
	enum vt_intensity intensity;
};

struct vt_data{
	unsigned short id;
	unsigned char vt_is_text_mode;

	struct vt_state state, saved_state;
	const struct consw* vt_sw;

	unsigned int vt_cols;
	unsigned int vt_rows;
	unsigned int vt_size_row;
	unsigned int vt_scan_lines;
	unsigned int vt_cell_height;

	unsigned long vt_screen_start;
	unsigned long vt_screen_end;
	unsigned long vt_visible_origin;

	/* Scrolling region */
	unsigned int vt_top, vt_bottom;

	/* In-memory character/color buffer */
	unsigned short* screenbuffer;
	unsigned long screenbuffer_size;
	unsigned long scrennbuffer_pos;

	unsigned short vt_erase_char;
	unsigned short vt_erase_attr;

	unsigned char vt_default_color;
	unsigned char vt_attr;

	struct font_info font;
};

struct vt {
	struct vt_data* data;
};

#endif
