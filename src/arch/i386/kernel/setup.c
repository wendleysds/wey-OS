#include <wey/printk.h>
#include <mm/memblock.h>
#include <def/init.h>
#include <def/config.h>
#include <def/linker.h>
#include <asm/cpu.h>
#include <asm/gdt.h>
#include <asm/idt.h>
#include <asm/page.h>
#include <wey/syscall.h>
#include <arch/i386/pic.h>
#include <lib/string.h>

#include "e820.h"
#include <uapi/headers.h>

#define ALIGN(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

struct boot_header boot_header;

unsigned long max_low_pfn_mapped;
unsigned long max_pfn_mapped;

unsigned long _brk_start = (unsigned long)__brk_base;
unsigned long _brk_ptr = (unsigned long)__brk_base;
unsigned long _brk_end = (unsigned long)__brk_limit;

static struct cpu cpus[MAX_CPUS];
static struct gdt_entry gdt[TOTAL_GDT_SEGMENTS];
static struct gdt_descriptor gdt_descriptor;

extern void fault_init();

static inline void gdt_set_tss(int cpu, struct tss *tss){
	int idx = GDT_TSS_BASE_INDEX + cpu;

	gdt[idx] = GDT_ENTRY(
		(unsigned long)tss, 
		sizeof(struct tss),
		GDT_TSS_AVAILABLE, 
		GDT_FLAG_32BIT
	);

	printk("Setup: tss: loaded \"%#lx\" idx %d (%#x).\n", tss, idx, GDT_TSS(idx));

	tss_load(GDT_TSS(idx));
}

static inline void tss_init(int cpu){
	if(cpu >= MAX_CPUS){
		return;
	}

	struct tss *t = &cpus[cpu].tss;
	memset(t, 0, sizeof(*t));
	t->ss0 = GDT_KERNEL_DATA;
	t->iopb = sizeof(struct tss);

	gdt_set_tss(cpu, t);
}

// no SMP yet, just use cpu 0

struct cpu* get_cpu(void){
	return &cpus[0];
}

__init void cpu_init(void){
	struct cpu *cpu = &cpus[0];
	cpu->id = 0;
	tss_init(cpu->id);
}

static __init void gdt_setup(){
	struct gdt_entry _gdt[TOTAL_GDT_SEGMENTS] = {
		[GDT_NULL_INDEX]        = GDT_ENTRY(0, 0, 0, 0),                                    // Null
		[GDT_KERNEL_CODE_INDEX] = GDT_ENTRY(0, 0xFFFFF, GDT_CODE_RING0, GDT_FLAGS_DEFAULT), // Kernel code
		[GDT_KERNEL_DATA_INDEX] = GDT_ENTRY(0, 0xFFFFF, GDT_DATA_RING0, GDT_FLAGS_DEFAULT), // Kernel data
		[GDT_USER_CODE_INDEX]   = GDT_ENTRY(0, 0xFFFFF, GDT_CODE_RING3, GDT_FLAGS_DEFAULT), // User code
		[GDT_USER_DATA_INDEX]   = GDT_ENTRY(0, 0xFFFFF, GDT_DATA_RING3, GDT_FLAGS_DEFAULT), // User data
	};

	memcpy(gdt, _gdt, sizeof(gdt));

	gdt_descriptor.addr = (uintptr_t)gdt;
	gdt_descriptor.size = sizeof(gdt) - 1;

	gdt_load(&gdt_descriptor);

	printk("Setup: gdt: loaded \"%#lx\".\n", &gdt_descriptor);
}

static __init void setup_memblock(void){
	// add usable memory
	for (size_t idx = 0; idx < boot_header.e820_entries_count; idx++){
		struct e820_entry* entry = &boot_header.e820_table[idx];

		if(entry->type == E820_TYPE_SOFT_RESERVED){
			memblock_reserve(entry->base_addr, entry->length);
		}

		if(entry->type != E820_TYPE_RAM){
			continue;
		}

		memblock_add(entry->base_addr, entry->length);
	}

	// reserve kernel
	memblock_reserve(
		__pa(__kernel_text_start), 
		(size_t)__end_of_kernel_reserve - (size_t)__kernel_text_start
	);

	// initial stack
	memblock_reserve(
		EARLY_STACK_BOTTOM,
		EARLY_STACK_SIZE 
	);

	// BIOS owned area
	memblock_reserve(0x0, KiB(64));

	// BRK
	if(_brk_ptr > _brk_start){
		memblock_reserve(__pa(_brk_start), _brk_ptr - _brk_start);
	}
}

__init void* extend_brk(size_t size, size_t align){
	_brk_end = ALIGN(_brk_end, align);

	void* ret = (void*)_brk_end;
	_brk_end += size;

	memset(ret, 0, size);
	return ret;
}

__init void setup_arch(void){
	memset(cpus, 0x0, sizeof(cpus));
	for(uint8_t i = 0; i < MAX_CPUS; i++){
		cpus[i].id = -1;
	}

	gdt_setup();

	cpu_init();

	pic_init(TIMER_FREQUENCY);

	idt_init();

	fault_init();

	e820_init(
		boot_header.e820_table, 
		boot_header.e820_entries_count
	);

	printk("Setup: E820-provided physical RAM map:\n");
	e820_print();

	max_pfn = e820_end_ram_pfn(MAX_ARCH_PFN);

	setup_memblock();

	memblock_dump_all();

	syscalls_init();
}
