#ifndef _TERMINAL_H
#define _TERMINAL_H

#include <device/terminal_struct.h>
#include <stdint.h>

#define TERMINAL_DEFAULT_COLOR TERMINAL_COLOR_LIGHT_GREY

#define VT_FG(c) ((c) & 0x0F)
#define VT_BG(c) (((c) >> 4) & 0x0F)

#define VT_COLOR(fg, bg) (((bg) << 4) | (fg))

struct vt_data;
struct font_info;
struct video_info;

typedef enum {
	TERMINAL_COLOR_BLACK =          0x0,
	TERMINAL_COLOR_BLUE =           0x1,
	TERMINAL_COLOR_GREEN =          0x2,
	TERMINAL_COLOR_CYAN =           0x3,
	TERMINAL_COLOR_RED =            0x4,
	TERMINAL_COLOR_MAGENTA =        0x5,
	TERMINAL_COLOR_BROWN =          0x6,
	TERMINAL_COLOR_LIGHT_GREY =     0x7,
	TERMINAL_COLOR_DARK_GREY =      0x8,
	TERMINAL_COLOR_LIGHT_BLUE =     0x9,
	TERMINAL_COLOR_LIGHT_GREEN =    0xA,
	TERMINAL_COLOR_LIGHT_CYAN =     0xB,
	TERMINAL_COLOR_LIGHT_RED =      0xC,
	TERMINAL_COLOR_LIGHT_MAGENTA =  0xD,
	TERMINAL_COLOR_YELLOW =         0xE,
	TERMINAL_COLOR_WHITE =          0xF,
} terminal_color_t;

typedef enum {
	TERMINAL_SCROLL_UP,
	TERMINAL_SCROLL_DOWN,
} terminal_scroll_t;

struct consw {
	int (*setup)(struct video_info* video_info);
	void (*init)(struct vt_data *vt);
	void (*exit_early)();
	void (*clean)(struct vt_data *vt);
	void (*clear_area)(struct vt_data *vt, unsigned int start_y, unsigned int start_x, unsigned int count);
	void (*putc)(struct vt_data *vt, uint16_t ca, unsigned int x, unsigned int y);
	void (*puts)(struct vt_data *vt, uint16_t* s, unsigned int start_x, unsigned int start_y);
	void (*cursor)(struct vt_data *vt, uint8_t enabled);
	void (*scroll)(struct vt_data *vt, unsigned int top, unsigned int bottom, terminal_scroll_t scroll_dir, unsigned int lines);
	int (*font_set)(struct vt_data *vt, struct font_info* font);
	int (*notify_switch)(struct vt_data *vt);
	struct font_info* (*font_get)(struct vt_data *vt);
	uint8_t	(*build_attr)(struct vt_data *vt, uint8_t color, enum vt_intensity intensity);
};

static inline uint8_t color_base(terminal_color_t c){
	return c & 0x7;
}

static inline enum vt_intensity color_intensity(terminal_color_t c){
	if (c & 0x8)
		return TERMINAL_INTENSITY_BOLD;
	return TERMINAL_INTENSITY_NORMAL;
}

int terminal_early_init();

int terminal_init();
void terminal_clean(struct vt_data* vt);
void terminal_puts(struct vt_data* vt, const char* s);
void terminal_scroll(struct vt_data *vt, unsigned int top, unsigned int bottom, terminal_scroll_t scroll_dir, unsigned int lines);
int terminal_font_set(struct vt_data *vt, struct font_info* font);
struct font_info* terminal_font_get(struct vt_data *vt);

#endif
