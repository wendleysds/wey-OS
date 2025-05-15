#include "core/kernel.h"
#include <drivers/terminal.h>
#include <lib/utils.h>
#include <lib/mem.h>
#include <io.h>
#include <stdint.h>
#include <stdarg.h>

/*
 * Main output module
 *
 * TODO: Rewrite to be a "Wrapper" VGA and VESA methods
 *
 * like:
 *
 * #ifdef USE_VGA
 * 	#define terminal_putchar(c, x, y, color) vga_putchar(c, x, y, color)
 * #else
 *	#define terminal_putchar(c, x, y, color) vesa_putchar(c, x, y, color)
 * #endif
 *
 */

#define DEFAULT_CURSOR_START 11
#define DEFAULT_CURSOR_END 12

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

/*void putpixel(struct VideoStructPtr* v, int x, int y, uint16_t color) {
	uint16_t* pixel = (uint16_t*)(v->framebuffer_physical + y * v->pitch + x * sizeof(uint16_t));
	*pixel = color;
}

void _putpixel(struct VideoStructPtr* v, int x, int y, uint16_t color){
	int index = (y * v->pitch + x * 2) / 2;
	videoMemory[index] = color;
}*/

void terminal_init(){
	struct VideoStructPtr* videoInfoPtr = (struct VideoStructPtr*)0x8000;
	memcpy(&video, videoInfoPtr, sizeof(struct VideoStructPtr));

	cursor.y = 0;
	cursor.x = 0;
	cursor.enabled = 1;

	videoMemory = (uint16_t*)video.framebuffer_physical;
	
	// fill screen with Withe
	uint32_t screenSize = video.width * video.height;
	for(uint32_t i = 0; i < screenSize; i++){
		videoMemory[i] = 0xFFFF;
	}

	int xOffset = 20;
	int yOffset = 20;

	int color[] = {0x0, 0x6D9D, 0x018C, 0x31B3, 0x5D7D, 0x3B5C, 0x2A97, 0x2236, 0x0};

	int c = 0;
	int loops = 8;
	while(loops > 0){
		for(uint32_t x = xOffset; x < video.width - xOffset; x++)
			for(uint32_t y = yOffset; y < video.height - yOffset; y++)
				putpixel(&video, x, y, color[c]);

		xOffset += 20;
		yOffset += 20;

		c++;
		loops--;
	}

}

void display_video_info(){
	terminal_write(0x0F, "--video-info--\n");
	terminal_write(0x0F, "Mode:        0x%x\n", video.mode);	
	terminal_write(0x0F, "Resolution:  %dx%dx%d\n", video.width, video.height, video.bpp);
	terminal_write(0x0F, "Pitch        0x%x\n", video.pitch);
	terminal_write(0x0F, "isGraphical: %s\n", video.isGraphical ? "true" : "false");
	terminal_write(0x0F, "isVesa:      %s\n", video.isVesa ? "true" : "false");
	terminal_write(0x0F, "--------------\n\n");
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

void terminal_cursor_enable(){
	terminal_cursor_enable_SE(DEFAULT_CURSOR_START, DEFAULT_CURSOR_END);
}

void terminal_cursor_enable_SE(uint8_t cursor_start, uint8_t cursor_end){
	if(cursor_start > cursor_end){
		terminal_write(0x04, "error enabling cursor!\n   ");
		terminal_write(TERMINAL_DEFAULT_COLOR, "'cursor_start' must be greater than 'cursor_end'");
		return;
	}

	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);

	cursor.enabled = 1;
	update_cursor();
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

void terminal_putchar(char c, unsigned char color) {
	if (c == '\n') {
      cursor.y += 1;
      cursor.x = 0;
	}
	else if(c == '\r'){
		cursor.x = 0;
	}
	else if(c == '\b'){
		if(cursor.x == 0){
			return;
		}

		cursor.x--;
		terminal_putchar(' ', color);
		cursor.x--;
	}
	else {
      int index = (cursor.y * video.width + cursor.x) * 2;
      videoMemory[index] = c;
      videoMemory[index + 1] = color;

      cursor.x += 1;
      if (cursor.x >= video.width) {
          cursor.y += 1;
          cursor.x = 0;
		}
	}
	
	scroll_terminal();
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

void terminal_vwrite(unsigned char color, const char *format, va_list args) {
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

void terminal_write(unsigned char color, const char *format, ...) {
  va_list args;
  va_start(args, format);
  terminal_vwrite(color, format, args);
  va_end(args);
}

