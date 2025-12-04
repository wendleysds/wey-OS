#ifndef _BOOT_HEADER_H
#define _BOOT_HEADER_H

#include <wey/videoinfo.h>
#include <stdint.h>

struct setup_header{
	uint8_t setup_sectors;
	uint32_t syssize;
	uint16_t video_mode;
	uint16_t sig;
	uint16_t root_device;
	uint16_t code16_start;
	uint32_t code32_start;
	uint64_t pref_address;
} __attribute__ ((packed));

struct boot_header{
	struct video_info video_info;
	uint8_t pad[16 - (sizeof(struct video_info) % 16)]; // Align 16 bytes
	struct setup_header setup_header;
} __attribute__ ((packed));

#endif
