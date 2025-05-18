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

static struct Cursor cursor;
static struct VideoStructPtr video;
static uint16_t* videoMemory = 0x0;

void putpixel(struct VideoStructPtr* v, uint16_t x, uint16_t y, uint32_t color){
	uint32_t offset = y * v->pitch + x * (v->bpp / 8);
	uint8_t* framebuffer = (uint8_t*)v->framebuffer_physical;

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

static void font_printchar(const unsigned char* font, const char c, uint8_t fontWidth, uint8_t fontHeigth){
	uint16_t x = cursor.x * fontWidth;
	uint16_t y = cursor.y * fontHeigth;

	for (uint8_t column = 0; column < fontHeigth; column++){
		//unsigned char line = font[(int)c][column];
		unsigned char bitmapLine = font[(int)c * fontHeigth + column];
		for (uint8_t row = 0; row < fontWidth; row++){
			unsigned char mask = 0x80 >> row;
			if(bitmapLine & mask)
				putpixel(&video, x, y, 0xFFFFFFFF);

			x++;
		}

		x = cursor.x * fontWidth;
		y++;
	}
}


void terminal_init(){
	struct VideoStructPtr* videoInfoPtr = (struct VideoStructPtr*)0x8000;
	memcpy(&video, videoInfoPtr, sizeof(struct VideoStructPtr));

	cursor.y = 0;
	cursor.x = 0;
	cursor.enabled = 1;

	videoMemory = (uint16_t*)video.framebuffer_physical;
}

void display_video_info(){
	terminal_write("--video-info--\n");
	terminal_write("Mode:        0x%x\n", video.mode);	
	terminal_write("Resolution:  %dx%dx%d\n", video.width, video.height, video.bpp);
	terminal_write("Pitch        0x%x\n", video.pitch);
	terminal_write("isGraphical: %s\n", video.isGraphical ? "true" : "false");
	terminal_write("isVesa:      %s\n", video.isVesa ? "true" : "false");
	terminal_write("--------------\n\n");
}

void update_cursor() {
	if(!cursor.enabled)
		return;

    uint16_t pos = cursor.y * video.width + cursor.x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_cursor_disable(){
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
	cursor.enabled = 0;
}

void scroll_terminal(){
	if(cursor.y >= video.height){
		for(int y = 1; y < video.height; y++){
			for(int x = 0; x < video.width; x++){
				int from = (y * video.width + x) * 2;
				int to = ((y - 1) * video.width + x) * 2;

				videoMemory[to] = videoMemory[from]; // Copy char
				videoMemory[to + 1] = videoMemory[from + 1]; // Copy color
			}
		}

		// Clear last line
		for(int x = 0; x < video.width; x++){
			int index = ((video.height - 1) * video.width + x) * 2;
			videoMemory[index] = ' ';
			videoMemory[index + 1] = 0x0F;
		}

		cursor.y = video.height - 1; // Adjust to last visible line
	}
}

void terminal_clear() {
	for(int i = 0; i < video.width * video.height; i++){
		videoMemory[i * 2] = ' ';
		videoMemory[i * 2 + 1] = 0x0F;
	}
	cursor.x = 0;
	cursor.y = 0;
	update_cursor();
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
		font_printchar((const unsigned char *)font8x16, c, 8, 16);

		cursor.x++;
    if (cursor.x >= video.width) {
			cursor.y += 1;
			cursor.x = 0;
		}
	}
	
	//scroll_terminal();
}

void terminal_backspace(){
	if(cursor.x == 0 && cursor.y != 0)
		return;

	if(cursor.x == 0){
		cursor.y -= 1;
		cursor.x = video.width;
	}
	
	cursor.x -= 1;
	terminal_putchar(' ', TERMINAL_DEFAULT_COLOR);
	cursor.x -= 1;
	update_cursor();
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

	update_cursor();
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

