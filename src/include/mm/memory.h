#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H

#include <def/config.h>

#define PFN_SECTION_SHIFT (VMEMMAP_SECTION_SHIFT - PAGE_SHIFT)
#define PFN_PER_SECTION (1UL << PFN_SECTION_SHIFT)

#define MAX_NR_MEM_SECTIONS  (MAX_ARCH_PFN >> (VMEMMAP_SECTION_SHIFT - PAGE_SHIFT))

#define pfn_to_section_nr(pfn) ((pfn) >> (VMEMMAP_SECTION_SHIFT - PAGE_SHIFT))

int memory_init(void);

#endif
