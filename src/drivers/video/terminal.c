#include "core/kernel.h"
#include <drivers/terminal.h>
#include <lib/font.h>
#include <lib/utils.h>
#include <lib/mem.h>
#include <io.h>
#include <stdint.h>
#include <stdarg.h>

/*
 * Main video stdout module
 */

#define DEFAULT_TAB_DISTANCE 4

#define BACKGROUND_COLOR 0x0

static struct Cursor cursor;
static struct VideoStructPtr video;

void vesa_putpixel(struct VideoStructPtr* v, uint16_t x, uint16_t y, uint32_t color){
	uint32_t offset = y * v->pitch + x * (v->bpp / 8);
	uint8_t* framebuffer = (uint8_t*)v->framebuffer_virtual;

	switch (v->bpp) {
		case 8:
			framebuffer[offset] = (uint8_t)color;
			break;
		case 15: // RGB555
    case 16: // RGB565
			*(uint16_t*)(framebuffer + offset) = (uint16_t)color;
			break;
		case 24:
			framebuffer[offset + 0] = (uint8_t)(color & 0xFF);
			framebuffer[offset + 1] = (uint8_t)((color >> 8) & 0xFF);
			framebuffer[offset + 2] = (uint8_t)((color >> 16) & 0xFF);
			break;
		case 32:
			*(uint32_t*)(framebuffer + offset) = color;
			break;
		default:
			panic("Video: Unsupported bpp");
			break;
	}
}

static void vesa_printchar(const unsigned char* font, const char c, uint8_t fontWidth, uint8_t fontHeigth){
	uint16_t x = cursor.x * fontWidth;
	uint16_t y = cursor.y * fontHeigth;

	for (uint8_t column = 0; column < fontHeigth; column++){
		//unsigned char line = font[(int)c][column];
		unsigned char bitmapLine = font[(int)c * fontHeigth + column];
		for (uint8_t row = 0; row < fontWidth; row++){
			unsigned char mask = 0x80 >> row;
			if(bitmapLine & mask)
				vesa_putpixel(&video, x, y, 0xFFFFFFFF);

			x++;
		}

		x = cursor.x * fontWidth;
		y++;
	}
}

struct VideoStructPtr* _get_video(){
	return &video;
}

void terminal_init(){
	struct VideoStructPtr* videoInfoPtr = (struct VideoStructPtr*)0x8000;
	memcpy(&video, videoInfoPtr, sizeof(struct VideoStructPtr));

	cursor.y = 0;
	cursor.x = 0;
	cursor.enabled = 1;

	video.framebuffer_virtual = video.framebuffer_physical;
}

void display_video_info(){
	terminal_write("--video-info--\n");
	terminal_write("Mode:        0x%x\n",     video.mode);	
	terminal_write("Resolution:  %dx%dx%d\n", video.width, video.height, video.bpp);
	terminal_write("Pitch        0x%x\n",     video.pitch);
	terminal_write("isGraphical: %s\n",       video.isGraphical ? "true" : "false");
	terminal_write("isVesa:      %s\n",       video.isVesa ? "true" : "false");
	terminal_write("--------------\n\n");
}

void scroll_terminal(){
	// TODO: Implement
}

void terminal_clear() {
	for(uint16_t x = 0; x < video.width; x++)
		for(uint16_t y = 0; y < video.height; y++)
			vesa_putpixel(&video, x, y, BACKGROUND_COLOR);

	cursor.x = 0;
	cursor.y = 0;
}

void terminal_putchar(char c, uint32_t color) {
	if (c == '\n') {
		cursor.y += 1;
		cursor.x = 0;
	}
	else if(c == '\r'){
		cursor.x = 0;
	}
	else if(c == '\b'){
		terminal_backspace();
	}
	else{
		vesa_printchar((const unsigned char *)font8x16, c, 8, 16);

		cursor.x++;
    if (cursor.x >= (video.width / 8)) {
			cursor.y += 1;
			cursor.x = 0;
		}
	}
	
	scroll_terminal();
}

void terminal_backspace(){
	// TODO: Implement
}


void terminal_vwrite(uint32_t color, const char *format, va_list args) {
  char buffer[32];

  for (const char *ptr = format; *ptr; ptr++) {
		if (*ptr == '%') {
			ptr++;
			switch (*ptr) {
				case 'c':
					terminal_putchar(va_arg(args, int), color);
          break;
        case 's':
        	for (char *s = va_arg(args, char*); *s; s++)
						terminal_putchar(*s, color);
          break;
        case 'd':
          itoa(va_arg(args, int), buffer, 10);
          for (char *s = buffer; *s; s++)
            terminal_putchar(*s, color);
          break;
        case 'x':
          itoa(va_arg(args, int), buffer, 16);
          for (char *s = buffer; *s; s++)
            terminal_putchar(*s, color);
          break;
        default:
          terminal_putchar('%', color);
          terminal_putchar(*ptr, color);
          break;
			}
		} else {
			terminal_putchar(*ptr, color);
    }
  }
}

void terminal_write(const char *format, ...){
	va_list args;
  va_start(args, format);
  terminal_vwrite(TERMINAL_DEFAULT_COLOR, format, args);
  va_end(args);
}

void terminal_cwrite(uint32_t color, const char *format, ...) {
  va_list args;
  va_start(args, format);
  terminal_vwrite(color, format, args);
  va_end(args);
}

