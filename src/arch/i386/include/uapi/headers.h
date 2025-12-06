#ifndef _BOOT_HEADER_H
#define _BOOT_HEADER_H

#include <def/compile.h>
#include <wey/videoinfo.h>
#include <stdint.h>
#include <asm/bootparam.h>

#define E820_MAX_ENTRIES 32

struct e820_entry{
	uint64_t base_addr;
	uint64_t length;
	uint32_t type;
} __packed;

struct efi_info {
	uint32_t signature;
	uint32_t systab_low;
	uint32_t systab_hig;
	uint32_t memdesc_size;
	uint32_t memdesc_version;
	uint32_t memmap_low;
	uint32_t memmap_hig;
	uint32_t memmap_size;
};

struct setup_header{
	uint8_t  setup_sectors;
	uint32_t syssize;
	uint16_t video_mode;
	uint32_t sig; // cAfE
	uint16_t root_device;
	uint16_t code16_start;
	uint32_t code32_start;
	uint32_t ramdisk_ptr;
	uint32_t ramdisk_size;
	uint8_t  ext_loader_ver;
	uint8_t  ext_loader_type;
	uint32_t kernel_alignment;
	uint8_t  relocatable_kernel;
	uint8_t  min_alignment;
	uint64_t pref_address;
} __packed;

struct boot_header{
	struct video_info video_info;     // 0x0
	uint8_t pad1[(16 - (sizeof(struct video_info) % 16)) % 16]; // Align 16 bytes
	uint64_t acpi_rsdp_addr;          // 0x30
	struct setup_header setup_header; // 0x38
	struct efi_info efi_info;         // 0x48
	uint8_t secure_boot;              // 0x68
	uint8_t e820_entries_count;       // 0x69
	uint8_t pad2[2];                  // 0x6A
	struct e820_entry e820_table[E820_MAX_ENTRIES]; // 0x6C
} __packed;

#endif
