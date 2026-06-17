#ifndef _BOOT_HEADER_H
#define _BOOT_HEADER_H

#include <def/compile.h>
#include <stdint.h>

#define BOOT_TAG_ALIGN 16

#define VIDEO_TYPE_MDA  0x10 // Monochrome Text Display
#define VIDEO_TYPE_CGA  0x11 // CGA Display
#define VIDEO_TYPE_EGAM 0x20 // EGA/VGA in Monochrome Mode
#define VIDEO_TYPE_EGAC 0x21 // EGA in Color Mode
#define VIDEO_TYPE_VGAC 0x22 // VGA+ in Color Mode
#define VIDEO_TYPE_VLFB 0x23 // VESA VGA in graphic mode
#define VIDEO_TYPE_EFI  0x70 // EFI graphic mode

#define BOOT_HEADER_SIGNATURE 0x63416645 // cAfE

enum boot_tags {
	BOOT_TAG_END = 0,
	BOOT_TAG_SETUP = 1,
	BOOT_TAG_E820,
	BOOT_TAG_VIDEO,
};

struct boot_tag {
	uint32_t type;
	uint32_t size;
} __packed;

enum e820_type {
	E820_TYPE_RAM           = 1,
	E820_TYPE_RESERVED      = 2,
	E820_TYPE_ACPI          = 3,
	E820_TYPE_NVS           = 4,
	E820_TYPE_UNUSABLE      = 5,
	E820_TYPE_PMEM          = 7,
	E820_TYPE_PRAM          = 12,
	E820_TYPE_SOFT_RESERVED = 0xefffffff,
};

struct e820_entry{
	uint64_t base_addr;
	uint64_t length;
	uint32_t type;
} __packed;

struct boot_tag_e820 {
	struct boot_tag tag;
	uint32_t entries;
	struct e820_entry map[];
} __packed;

struct boot_tag_video {
	struct boot_tag tag;
	uint16_t mode;

	uint8_t type;
	uint8_t bpp;
	uint16_t width;
	uint16_t height;
	uint16_t pitch;
	uint64_t framebuffer;

	uint16_t red_mask;
	uint16_t red_position;

	uint16_t green_mask;
	uint16_t green_position;

	uint16_t blue_mask;
	uint16_t blue_position;

	uint16_t reserved_mask;
	uint16_t reserved_position;

	uint16_t attributes;
	uint32_t capabilities;
	uint32_t vesapm_addr;
} __packed;

struct boot_tag_setup{
	struct boot_tag	tag;

	uint8_t setup_sectors;
	uint8_t pad1[3];

	uint32_t syssize;
	uint32_t code16_start;
	uint32_t code32_start;

	uint32_t kernel_alignment;

	uint8_t relocatable_kernel;
	uint8_t min_alignment;
	uint16_t boot_sig;
	uint64_t pref_address;
} __packed;

struct boot_header {
	uint32_t signature;

	uint16_t version;
	uint16_t size;

	uint32_t checksum;

	uint64_t tags_ptr;
	uint64_t tags_size;

	uint8_t secure_boot;
	uint8_t reserved[7];
} __packed;

static inline struct boot_tag* boot_tag_next(const struct boot_tag* tag){
	uintptr_t aligned = (tag->size + (BOOT_TAG_ALIGN - 1)) & ~(BOOT_TAG_ALIGN - 1);
	return (struct boot_tag*)((char*)tag + aligned);
}

static inline const struct boot_tag* boot_find_tag(
	const struct boot_header *hdr, 
	enum boot_tags type
){
	const struct boot_tag *tag = (void*)hdr->tags_ptr;
	while(tag->type != BOOT_TAG_END) {
		if(tag->type == type)
			return tag;

		tag = boot_tag_next(tag);
	}

	return 0x0;
}

#endif
