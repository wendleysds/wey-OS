#ifndef _GENERIC_PAGING_OPS_H
#define _GENERIC_PAGING_OPS_H

#include <asm/paging.h>
#include <stdint.h>

struct paging_ops {
	pte_t   (*mk_pte)(uintptr_t phys, uint32_t flags);
	uintptr_t (*pte_phys)(pte_t pte);

	int (*pte_present)(pte_t pte);
	int (*pte_leaf)(pte_t pte);

	void (*set_pte)(pte_t *dst, pte_t val);
	void (*clear_pte)(pte_t *dst);

	void* (*pte_to_virt)(pte_t pte);
	pte_t (*mk_table)(uintptr_t table_virt);

	void (*flush_tlb_one)(uintptr_t vaddr);
	void (*flush_all)(void);
};

#endif
