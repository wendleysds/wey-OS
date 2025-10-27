#ifndef _BOOT_HEADER_H
#define _BOOT_HEADER_H

#include <wey/videoinfo.h>
#include <stdint.h>

struct setup_header{
	uint8_t setup_sectors;
	uint16_t video_mode;
	uint16_t root_device;
	uint32_t code32_start;
	uint64_t pref_address;
} __attribute__ ((packed));

struct boot_header{
	struct video_info video_info;
	struct setup_header setup_header;
} __attribute__ ((packed));

#endif
