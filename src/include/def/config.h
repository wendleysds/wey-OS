#ifndef _GLOBAL_CONFIG_H
#define _GLOBAL_CONFIG_H

#include <arch-config.h>

/* Interrupts */
#define TOTAL_INTERRUPTS 256

/* Kernel load bases */
#define KERNEL_PHYS_BASE 0x00100000
#define KERNEL_VIRT_BASE 0xC0000000

/* MMU */
#define PAGE_OFFSET KERNEL_VIRT_BASE

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
            | Fixed Mapping                          |
            +----------------------------------------+
            | Sparse Vmemmap                         |
            +----------------------------------------+
            | Highmem                                |
            +----------------------------------------+
            | NON-LINEAR / MMIO                      |
0xFFFFFFFF  +----------------------------------------+
*/

// Limits
#define KERNEL_NONLINEAR_SIZE MiB(32)
#define KERNEL_HIGHMEM_SIZE MiB(16)
#define KERNEL_VMEMMAP_SIZE MiB(32)
#define KERNEL_FIXEDMAP_SIZE MiB(8)
#define KERNEL_DIRECTMAP_SIZE ( \
	GiB(1) - KERNEL_HIGHMEM_SIZE - KERNEL_FIXEDMAP_SIZE - \
	KERNEL_NONLINEAR_SIZE - KERNEL_VMEMMAP_SIZE \
)

// Vmemmap
#define VMEMMAP_SECTION_SHIFT 26
#define VMEMMAP_SECTION_SIZE (1UL << VMEMMAP_SECTION_SHIFT)
#define VMEMMAP_PFN_SECTION_MASK (~((VMEMMAP_SECTION_SIZE / PAGE_SIZE) - 1)) 

// Addresses
#define USER_SPACE_START (0x00000000)
#define USER_SPACE_END   (KERNEL_VIRT_BASE - 1)

#define KERNEL_DIRECTMAP_START (KERNEL_VIRT_BASE)
#define KERNEL_DIRECTMAP_END   (KERNEL_DIRECTMAP_START + KERNEL_DIRECTMAP_SIZE - 1)

#define KERNEL_FIXEDMAP_START (KERNEL_DIRECTMAP_END + 1)
#define KERNEL_FIXEDMAP_END   (KERNEL_FIXEDMAP_START + KERNEL_FIXEDMAP_SIZE - 1)

#define KERNEL_VMEMMAP_START (KERNEL_FIXEDMAP_END + 1)
#define KERNEL_VMEMMAP_END   (KERNEL_VMEMMAP_START + KERNEL_VMEMMAP_SIZE - 1)

#define KERNEL_HIGHMEM_START (KERNEL_VMEMMAP_END + 1)
#define KERNEL_HIGHMEM_END   (KERNEL_HIGHMEM_START + KERNEL_HIGHMEM_SIZE - 1)

#define KERNEL_NONLINEAR_START (KERNEL_HIGHMEM_END)
#define KERNEL_NONLINEAR_END ((KERNEL_NONLINEAR_START + KERNEL_NONLINEAR_SIZE) - 1) 

#define EARLY_STACK_BOTTOM 0x90000
#define EARLY_STACK_SIZE KiB(16)
#define EARLY_STACK_ADDR (EARLY_STACK_BOTTOM + EARLY_STACK_SIZE)

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
