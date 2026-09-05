#ifndef _BOOT_H
#define _BOOT_H

#include <asm/cpuflags.h>
#include <uapi/headers.h>
#include <def/compile.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define SEG(ptr) (uint16_t)(((uint32_t)(ptr) >> 4) & 0xFFFF)
#define OFF(ptr) (uint16_t)((uint32_t)(ptr) & 0xF)
#define FAR_PTR(seg, off) ((void*)(((uint32_t)(seg) << 4) + (off)))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#define memset(d,c,l) __builtin_memset(d,c,l)
#define memcpy(d,s,l) __builtin_memcpy(d,s,l)

struct biosregs {
	union {
		struct {
			uint32_t edi;
			uint32_t esi;
			uint32_t ebp;
			uint32_t _esp;
			uint32_t ebx;
			uint32_t edx;
			uint32_t ecx;
			uint32_t eax;
			uint32_t _fsgs;
			uint32_t _dses;
			uint32_t eflags;
		};
		struct {
			uint16_t di, hdi;
			uint16_t si, hsi;
			uint16_t bp, hbp;
			uint16_t _sp, _hsp;
			uint16_t bx, hbx;
			uint16_t dx, hdx;
			uint16_t cx, hcx;
			uint16_t ax, hax;
			uint16_t gs, fs;
			uint16_t es, ds;
			uint16_t flags, hflags;
		};
		struct {
			uint8_t dil, dih, edi2, edi3;
			uint8_t sil, sih, esi2, esi3;
			uint8_t bpl, bph, ebp2, ebp3;
			uint8_t _spl, _sph, _esp2, _esp3;
			uint8_t bl, bh, ebx2, ebx3;
			uint8_t dl, dh, edx2, edx3;
			uint8_t cl, ch, ecx2, ecx3;
			uint8_t al, ah, eax2, eax3;
		};
	};
};

static inline uint16_t ds(void){
	uint16_t seg;
	asm("movw %%ds,%0" : "=rm" (seg));
	return seg;
}

static inline uint16_t fs(void){
	uint16_t seg;
	asm volatile("movw %%fs,%0" : "=rm" (seg));
	return seg;
}

static inline uint16_t gs(void){
	uint16_t seg;
	asm volatile("movw %%gs,%0" : "=rm" (seg));
	return seg;
}

// helper to access the whole memory

static inline void set_fs(uint16_t seg) {
	asm volatile("movw %0, %%fs" : : "rm" (seg));
}

static inline uint8_t rdfs8(uint32_t addr_fs) {
	uint8_t val;
	asm volatile("movb %%fs:(%1), %0" : "=q" (val) : "r" (addr_fs));
	return val;
}
static inline uint16_t rdfs16(uint32_t addr_fs) {
	uint16_t val;
	asm volatile("movw %%fs:(%1), %0" : "=r" (val) : "r" (addr_fs));
	return val;
}
static inline uint32_t rdfs32(uint32_t addr_fs) {
	uint32_t val;
	asm volatile("movl %%fs:(%1), %0" : "=r" (val) : "r" (addr_fs));
	return val;
}
static inline void wrfs8(uint8_t val, uint32_t addr_fs) {
	asm volatile("movb %0, %%fs:(%1)" : : "q" (val), "r" (addr_fs) : "memory");
}
static inline void wrfs16(uint16_t val, uint32_t addr_fs) {
	asm volatile("movw %0, %%fs:(%1)" : : "r" (val), "r" (addr_fs) : "memory");
}
static inline void wrfs32(uint32_t val, uint32_t addr_fs) {
	asm volatile("movl %0, %%fs:(%1)" : : "r" (val), "r" (addr_fs) : "memory");
}

void *memcpy_fromfs(void *dst_ds, uint32_t src_fs, size_t count);
void memcpy_tofs(uint32_t dst_fs, const void *src_ds, size_t count);
void *memset_fs(uint32_t dst_fs, int c, size_t count);
int memcmp_fs(uint32_t s1_fs, const void *s2_ds, size_t count);

extern struct boot_tag_setup hdr;

// BIOS
void initregs(struct biosregs *reg);
void __regparm(3) intcall(uint8_t int_no, const struct biosregs *ireg, struct biosregs *oreg);

// Setup
void setup_video(struct boot_tag_video* video) ;
int detect_memory(struct e820_entry* out_table);

// Keyboard
int kbd_getchar();
void kbd_flush();
int kdb_read(char* restrict buffer, int length);

// A20
int enable_a20();

// ACPI
int acpi_find_rsdp(struct boot_tag_acpi* acpi);

// stdio
void putchar(int c);
void puts(const char* restrict s);
int vsprintf(char* restrict buf, const char* restrict fmt, va_list args);
int sprintf(char* restrict buf, const char* restrict fmt, ...);
int printf(const char* restrict fmt, ...);

// pm
void __no_return __regparm(3) go_to_protect_mode(uint32_t entry_point, uint32_t boot_header);

#endif
