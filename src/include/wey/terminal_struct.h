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
	unsigned char vt_mode;

	struct vt_state state, saved_state;
	const struct video_ops* ops;

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

	void* screenbuffer;
	unsigned int screenbuffer_size;

	/* For all chars on screen */
	struct {
		
	} attributes;

	struct {

	} cursor;

	struct {

	} font;

	struct {

	} mode;
};

#endif
