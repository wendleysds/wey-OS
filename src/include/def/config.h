#ifndef _GLOBAL_CONFIG_H
#define _GLOBAL_CONFIG_H

/*GDT*/
#define TOTAL_GDT_SEGMENTS 6

#define GDT_NULL_INDEX        0 
#define GDT_KERNEL_CODE_INDEX 1
#define GDT_KERNEL_DATA_INDEX 2
#define GDT_USER_CODE_INDEX   3
#define GDT_USER_DATA_INDEX   4
#define GDT_TSS_INDEX         5

#define GDT_KERNEL_CODE  (GDT_KERNEL_CODE_INDEX << 3)
#define GDT_KERNEL_DATA  (GDT_KERNEL_DATA_INDEX << 3)
#define GDT_USER_CODE    ((GDT_USER_CODE_INDEX << 3) | 3)
#define GDT_USER_DATA    ((GDT_USER_DATA_INDEX << 3) | 3)
#define GDT_TSS          (GDT_TSS_INDEX << 3)

#ifndef __ASSEMBLY__
extern unsigned long __kernel_phys_base;
extern unsigned long __kernel_high_base;

extern unsigned long __kernel_phys_start;
extern unsigned long __kernel_phys_end;
extern unsigned long __kernel_high_start;
extern unsigned long __kernel_high_end;
#else
extern __kernel_phys_base
extern __kernel_high_base

extern __kernel_phys_start
extern __kernel_phys_end
extern __kernel_high_start
extern __kernel_high_end
#endif

#define PAGE_OFFSET      0xC0000000
#define KERNEL_PHYS_BASE 0x00100000

#ifndef __ASSEMBLY__
#define KERNEL_PHYS_START ((unsigned long)&__kernel_phys_start)
#define KERNEL_PHYS_END   ((unsigned long)&__kernel_phys_end)
#define KERNEL_VIRT_START ((unsigned long)&__kernel_high_start + KERNEL_PHYS_BASE)
#define KERNEL_VIRT_END   ((unsigned long)&__kernel_high_end   + KERNEL_PHYS_BASE)
#else
#define KERNEL_PHYS_START [__kernel_phys_start]
#define KERNEL_PHYS_END   [__kernel_phys_end]
#define KERNEL_VIRT_START [__kernel_high_start]
#define KERNEL_VIRT_END   [__kernel_high_end]
#endif

#define TOTAL_INTERRUPTS 256
#define TIMER_FREQUENCY 20

#define KiB(x) (x * 1024)
#define MiB(x) (x * KiB(1024))

#define ALIGN(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

/* Memory Layout

0x00000000  +----------------------------------------+
            | User Space                             |
0xC0000000  +----------------------------------------+ 0x100000
            | Kernel image (.text/.data/.bss)        |
            |                                        |
            | __kernel_high_start                    |
            | __kernel_high_end                      |
            +----------------------------------------+
            | Early allocator / init sections        |
			| Early Pages & Tables                   |
            | Page metadata (struct page[])          |
            +----------------------------------------+
            | Kernel stack + page guard (Per-CPU)    |
            +----------------------------------------+
            | Slab / kmalloc area (lowmem)           |
0xEFFFFFFF  +----------------------------------------+
            | NON-LINEAR / MMIO                      | ~256MiB
0xFFFFFFFF  +----------------------------------------+

*/

#define PAGE_SIZE 0x1000

#define RESERVED_SIZE       MiB(8)
#define KERNEL_STACKS_SIZE  MiB(1)

#define USER_SPACE_START    0x00000000UL
#define USER_SPACE_END      0xBFFFFFFFUL

#define KERNEL_MMIO_START   0xF0000000UL
#define KERNEL_MMIO_END     0xFFFFFFFFUL

#define KERNEL_EARLY_START  (ALIGN(KERNEL_VIRT_END, PAGE_SIZE))
#define KERNEL_EARLY_END    (KERNEL_EARLY_START + RESERVED_SIZE - 1)

#define KERNEL_STACKS_START  (KERNEL_EARLY_END + 1)
#define KERNEL_STACKS_END    (KERNEL_STACKS_START + KERNEL_STACKS_SIZE - 1)

#define KERNEL_HEAP_START   (ALIGN(KERNEL_STACKS_END + 1, PAGE_SIZE))
#define KERNEL_HEAP_END     (KERNEL_MMIO_START - 1)

#define KERNEL_MMIO_SIZE    (KERNEL_MMIO_END - KERNEL_MMIO_START)

#define EARLY_STACK_SIZE KiB(4)
#define EARY_STACK_ADDR_BOTTOM 0x90000
#define EARY_STACK_ADDR (EARY_STACK_ADDR_BOTTOM - EARLY_STACK_SIZE)

/*Files and File System*/
#define PATH_MAX 128
#define FILE_DESCRIPTORS_MAX 64
#define FILESYSTEMS_MAX 8

/*Devices*/
#define DEVICES_MAX 16
#define MAJOR_MAX 6
#define MINOR_MAX 8

/*Processes*/
#define PROC_MAX 32
#define PROC_NAME_MAX 32
#define PROC_ARG_MAX 32
#define PROC_FD_MAX 16

// Process Stack
#define PROC_USER_STACK_VIRUTAL_TOP 0x3FF000
#define PROC_USER_STACK_SIZE 8192
#define PROC_USER_STACK_VIRUTAL_BUTTOM (PROC_USER_STACK_VIRUTAL_TOP - PROC_USER_STACK_SIZE)

/*Terminal/Console*/
#define TERMINALS_MAX 12
#define VIDEO_DRIVERS_MAX 4

#endif
