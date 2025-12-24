#ifndef _TERMINAL_H
#define _TERMINAL_H

#include <stdint.h>
#include <stdarg.h>

#define TERMINAL_DEFAULT_COLOR TERMINAL_COLOR_LIGHT_GREY

struct vt_data;
struct font_info;
struct device;

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

enum terminal_scroll_direction{
	SCROLL_UP,
	SCROLL_DOWN,
};

struct video_ops {
	void (*init)(struct vt_data *vt);
	void (*clean)(struct vt_data *vt);
	void (*putc)(struct vt_data *vt, int ch, unsigned int x, unsigned int y, unsigned char color /*Fore & Back color*/);
	void (*cursor)(struct vt_data *vt, uint8_t enabled);
	void (*scroll)(struct vt_data *vt, unsigned int top, unsigned int bottom, enum terminal_scroll_direction dir);
	void (*font_set)(struct vt_data *vt, struct font_info* font);
	void (*font_get)(struct vt_data *vt, struct font_info* font);
	void (*resize)(struct vt_data *vt, unsigned int heigth, unsigned int width);
};

struct video_driver {
	const char* desc;
	const struct video_ops* ops;
	struct device *dev;
	int flags;
};

#endif
