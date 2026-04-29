#ifndef _GLOBAL_CONFIG_H
#define _GLOBAL_CONFIG_H

/*GDT*/
#define TOTAL_GDT_SEGMENTS 6

#define GDT_NULL_INDEX        0 
#define GDT_KERNEL_CODE_INDEX 1
#define GDT_KERNEL_DATA_INDEX 2
#define GDT_USER_CODE_INDEX   3
#define GDT_USER_DATA_INDEX   4
#define GDT_TSS_BASE_INDEX    5

#define GDT_KERNEL_CODE  (GDT_KERNEL_CODE_INDEX << 3)
#define GDT_KERNEL_DATA  (GDT_KERNEL_DATA_INDEX << 3)
#define GDT_USER_CODE    ((GDT_USER_CODE_INDEX << 3) | 3)
#define GDT_USER_DATA    ((GDT_USER_DATA_INDEX << 3) | 3)
#define GDT_TSS(index)   ((index) << 3)

/* PIC */
#define TIMER_FREQUENCY 20

/* Interrupts */
#define TOTAL_INTERRUPTS 256

/* Kernel load bases */
#define KERNEL_PHYS_BASE 0x00100000
#define KERNEL_VIRT_BASE 0xC0000000
#define PAGE_OFFSET KERNEL_VIRT_BASE

/* MMU */
#define PAGE_SIZE 0x1000

/* Helpers */
#define KiB(x) ((x) * 1024)
#define MiB(x) ((x) * KiB(1024))
#define GiB(x) ((x) * MiB(1024))

/* Memory Layout */

/*
0x00000000  +----------------------------------------+
            | User Space                             |
0xC0000000  +----------------------------------------+ 0x100000
            | Direct Mapping                         |
            +----------------------------------------+
            | Sparse Vmemmap                         |
            +----------------------------------------+
            | Highmem                                |
            +----------------------------------------+
            | Fixed Mapping                          |
            +----------------------------------------+
            | NON-LINEAR / MMIO                      | ~256MiB
0xFFFFFFFF  +----------------------------------------+
*/

#define KERNEL_NONLINEAR_MMIO_START (0xF0000000UL)
#define KERNEL_NONLINEAR_MMIO_END   (0xFFFFFFFFUL)

#define KERNEL_NONLINEAR_MMIO_SIZE (KERNEL_NONLINEAR_MMIO_END - KERNEL_NONLINEAR_MMIO_START)
#define KERNEL_HIGHMEM_SIZE MiB(16)
#define KERNEL_FIXEDMAP_SIZE MiB(8)
#define KERNEL_VMEMMAP_SIZE MiB(32)
#define KERNEL_DIRECTMAP_SIZE ( \
	GiB(1) - KERNEL_HIGHMEM_SIZE - KERNEL_FIXEDMAP_SIZE \
	KERNEL_NONLINEAR_MMIO_SIZE - KERNEL_VMEMMAP_SIZE \
)

#define RESERVED_SIZE       MiB(8)
#define KERNEL_STACKS_SIZE  MiB(1)

#define USER_SPACE_START (0x00000000)
#define USER_SPACE_END   (KERNEL_VIRT_BASE - 1)

#define KERNEL_DIRECTMAP_START (KERNEL_VIRT_BASE)
#define KERNEL_DIRECTMAP_END   (KERNEL_DIRECTMAP_START + KERNEL_DIRECTMAP_SIZE - 1)

#define KERNEL_VMEMMAP_START (KERNEL_DIRECTMAP_END + 1)
#define KERNEL_VMEMMAP_END   (KERNEL_VMEMMAP_START + KERNEL_VMEMMAP_SIZE - 1)

#define KERNEL_FIXEDMAP_START (KERNEL_VMEMMAP_END + 1)
#define KERNEL_FIXEDMAP_END   (KERNEL_FIXEDMAP_START + KERNEL_FIXEDMAP_SIZE - 1)

#define KERNEL_HIGHMEM_START (KERNEL_FIXEDMAP_END + 1)
#define KERNEL_HIGHMEM_END   (KERNEL_HIGHMEM_START + KERNEL_HIGHMEM_SIZE - 1)

#define EARY_STACK_ADDR 0x94000

/*Printk*/
#define PRINTK_BUFFER_SIZE KiB(16)

/*Files and File System*/
#define PATH_MAX 128
#define FILE_DESCRIPTORS_MAX 64
#define FILESYSTEMS_MAX 8

/*Devices*/
#define MAJOR_MAX 12
#define MINOR_MAX 12
#define DEVICES_MAX ((MAJOR_MAX + MINOR_MAX) * 2)

/*Processes*/
#define PROC_MAX 32
#define PROC_NAME_MAX 32
#define PROC_ARG_MAX 32
#define PROC_FD_MAX 16

// Process Stack
#define PROC_USER_STACK_VIRUTAL_TOP 0x3FF000
#define PROC_USER_STACK_SIZE 8192
#define PROC_KERNEL_STACK_SIZE 4096
#define PROC_USER_STACK_VIRUTAL_BUTTOM (PROC_USER_STACK_VIRUTAL_TOP - PROC_USER_STACK_SIZE)

/*Terminal/Console*/
#define TERMINALS_MAX 6

/*CPU*/
#define MAX_CPUS 4

#endif
