#ifndef _GDT_H
#define _GDT_H

// Attributes (Access byte)
#define GDT_ACCESS_PRESENT     (1 << 7)
#define GDT_ACCESS_RING(r)     ((r & 0x3) << 5)
#define GDT_ACCESS_DESCRIPTOR  (1 << 4) // 1 = code/data, 0 = system

// Segments type (bit 3..0)
#define GDT_TYPE_CODE          (1 << 3) // Executable
#define GDT_TYPE_DATA          (0 << 3) // Data (not executable)
#define GDT_TYPE_EXPAND_DOWN   (1 << 2) // For data
#define GDT_TYPE_CONFORMING    (1 << 2) // For code
#define GDT_TYPE_READABLE      (1 << 1) // Code: Can read
#define GDT_TYPE_WRITABLE      (1 << 1) // Data: Can write
#define GDT_TYPE_ACCESSED      (1 << 0)

// Extra Attributes (Flags byte, bits 55–52)
#define GDT_FLAG_GRANULARITY   (1 << 7) // 4 KiB granularity
#define GDT_FLAG_32BIT         (1 << 6) // 32-bit segment
#define GDT_FLAG_64BIT         (1 << 5) // 64-bit segment (if used)
#define GDT_FLAG_AVAILABLE     (1 << 4)

// Kernel code segment (ring 0)
#define GDT_CODE_RING0  ( \
	GDT_ACCESS_PRESENT | \
	GDT_ACCESS_RING(0) | \
	GDT_ACCESS_DESCRIPTOR | \
	GDT_TYPE_CODE | \
	GDT_TYPE_READABLE )

// Kernel data segment (ring 0)
#define GDT_DATA_RING0  ( \
	GDT_ACCESS_PRESENT | \
	GDT_ACCESS_RING(0) | \
	GDT_ACCESS_DESCRIPTOR | \
	GDT_TYPE_DATA | \
	GDT_TYPE_WRITABLE )

// User code segment (ring 3)
#define GDT_CODE_RING3  ( \
	GDT_ACCESS_PRESENT | \
	GDT_ACCESS_RING(3) | \
	GDT_ACCESS_DESCRIPTOR | \
	GDT_TYPE_CODE | \
	GDT_TYPE_READABLE )

// User data segment (ring 3)
#define GDT_DATA_RING3  ( \
	GDT_ACCESS_PRESENT | \
	GDT_ACCESS_RING(3) | \
	GDT_ACCESS_DESCRIPTOR | \
	GDT_TYPE_DATA | \
	GDT_TYPE_WRITABLE )

// TSS (system segment)
#define GDT_TSS_AVAILABLE ( \
	GDT_ACCESS_PRESENT | \
	GDT_ACCESS_RING(0) | \
	(0 << 4) | /* System segment */ \
	0x9 )      /* Type: 1001b -> TSS avalible */

#define GDT_FLAGS_DEFAULT (GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT)

#define GDT_KERNEL_CODE  (1 << 3)  // index 1
#define GDT_KERNEL_DATA  (2 << 3)  // index 2

#ifndef __ASSEMBLY__
#include <stdint.h>

#define GDT_ENTRY(_base, _limit, _access, _flags) (struct gdt_entry){ \
    .limit_low = (_limit) & 0xFFFF, \
    .base_low  = (_base) & 0xFFFF, \
    .base_mid  = ((_base) >> 16) & 0xFF, \
    .access    = (_access), \
    .granularity = (((_limit) >> 16) & 0x0F) | ((_flags) & 0xF0), \
    .base_high = ((_base) >> 24) & 0xFF \
}

struct gdt_descriptor{
	uint16_t size;
	uint32_t addr;
} __attribute__((packed));

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

static inline void gdt_load(struct gdt_descriptor* gdt_descriptor){
	__asm__ volatile ("lgdt (%0)" :: "r"(gdt_descriptor) : "memory");
}
#endif

#endif
