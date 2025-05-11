#ifndef _BOOT_VIDEO_H
#define _BOOT_VIDEO_H

#include <stdint.h>

struct VbeInfoBlock {
  char     VbeSignature[4];
  uint16_t VbeVersion;
  uint16_t OemStringPtr[2];
  uint8_t  Capabilities[4];
  uint16_t VideoModePtr[2];
  uint16_t TotalMemory;
  uint8_t  Reserved[492];
} __attribute__((packed));

struct VbeInfoMode {
	uint16_t attributes;
	uint8_t window_a;
	uint8_t window_b;
	uint16_t granularity;
	uint16_t window_size;
	uint16_t segment_a;
	uint16_t segment_b;
	uint32_t win_func_ptr;
	uint16_t pitch;
	uint16_t width;
	uint16_t height;
	uint8_t w_char;
	uint8_t y_char;
	uint8_t planes;
	uint8_t bpp;
	uint8_t banks;
	uint8_t memory_model;
	uint8_t bank_size;
	uint8_t image_pages;
	uint8_t reserved0;

	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;
	uint8_t direct_color_attributes;

	uint32_t framebuffer;
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;
	uint8_t reserved1[206];
} __attribute__ ((packed));

int check_video();
void set_video();

#endif
