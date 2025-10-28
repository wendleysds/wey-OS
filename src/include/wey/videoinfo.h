#ifndef _VIDEO_INFO_H
#define _VIDEO_INFO_H

#include <stdint.h>

#define VIDEO_TYPE_MDA  0x10 // Monochrome Text Display
#define VIDEO_TYPE_CGA  0x11 // CGA Display
#define VIDEO_TYPE_EGAM 0x20 // EGA/VGA in Monochrome Mode
#define VIDEO_TYPE_EGAC 0x21 // EGA in Color Mode
#define VIDEO_TYPE_VGAC 0x22 // VGA+ in Color Mode
#define VIDEO_TYPE_VLFB 0x23 // VESA VGA in graphic mode
#define VIDEO_TYPE_EFI  0x70 // EFI graphic mode

struct video_info
{
	uint8_t type;

	uint16_t height;
	uint16_t width;
	uint16_t pitch;
	uint8_t bpp; // 0 for text mode

	uintptr_t framebuffer;

	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;

	uint8_t direct_color_attributes;
	uint16_t attributes;
} __attribute__ ((packed));

#endif
