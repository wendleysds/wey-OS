#include <wey/mmu.h>
#include <wey/panic.h>
#include <asm/paging.h>
#include <mm/kheap.h>
#include <def/err.h>
#include <def/config.h>

#define ADDRS_NOT_ALING(virt, phys) \
	(((uintptr_t)(virt) & (PTE_PAGE_SIZE - 1)) || \
	((uintptr_t)(phys) & (PTE_PAGE_SIZE - 1)))

#define SELF_PDE_INDEX 1023
#define SELF_PDE_BASE  (SELF_PDE_INDEX << 22)

// pgd virtual table base address
#define PGD_VADDR ((pgd_t*)0xFFFFF000)

// pte virtual table base address
#define PT_VADDR(d) ((void*)(SELF_PDE_BASE + ((d) << 12)))

// pte virtual table address
#define PTE_VADDR(d,t) ((void*)(SELF_PDE_BASE + ((d) << 12) + ((t) << 2)))

// *PDE_VADDR(d) == PGD_VADDR[d]
#define PDE_VADDR(d) ((unsigned long*)(0xFFFFF000 + ((d) << 2))) 

pgd_t* _current_pgd = NULL;

int pgd_load(pgd_t* pgd){
	if(!pgd){
		return NULL_PTR;
	}

	if(_current_pgd == pgd){
		return SUCCESS;
	}

	if ((uintptr_t)(pgd) & (PTE_PAGE_SIZE - 1)) {
		return BAD_ALIGNMENT;
	}

	__asm__ volatile (
		"mov %0, %%cr3"
		:
		: "r"(pgd_translate(pgd))
		: "memory"
	);

	_current_pgd = pgd;
	
	return SUCCESS;
}

pgd_t* pgd_alloc(){
	pgd_t* pgd = (pgd_t*)kcalloc(PGD_MAX_PTE, sizeof(pgd_t));

	if(pgd){
		pgd[SELF_PDE_INDEX] = (pgd_t)pgd_translate((void*)pgd) | _PAGE_P | _PAGE_RW;
	}

	return pgd;
}

void pgd_free(pgd_t* pgd){
	if(pgd == _current_pgd){
		panic("pgd_free(): Trying to free current directory!");
	}

	pgd[SELF_PDE_INDEX] = 0;

	for (int i = 0; i < PGD_MAX_PTE; i++) {
		if (pgd[i] & _PAGE_P) {
			kfree_phys((void*)(pgd[i] & PAGE_MASK));
		}
	}

	kfree(pgd);	
}

int pgd_map(uintptr_t virtaddr, uintptr_t physaddr, mem_flags_t flags){
	if(ADDRS_NOT_ALING(virtaddr, physaddr)){
		return BAD_ALIGNMENT;
	}

	uint32_t dir_idx = virtaddr >> 22;
	uint32_t tbl_idx = (virtaddr >> 12) & 0x3FF;

	pgd_t* pde = PGD_VADDR;
	pte_t* pte = PT_VADDR(dir_idx);

	uint8_t pg_flags = _PAGE_P;
	if(flags & MEM_WRITE)   pg_flags |= _PAGE_RW;
	if(flags & MEM_USER)    pg_flags |= _PAGE_US;

	if (!(pde[dir_idx] & _PAGE_P)) {
		void* newTable = kcalloc(PTE_MAX_ENTRIES, sizeof(pte_t));
		if (!newTable) {
			return NO_MEMORY;
		}

		pde[dir_idx] = (pte_t)pgd_translate(newTable) | pg_flags;
		pte = PT_VADDR(dir_idx);
	}

	if (pte[tbl_idx] & _PAGE_P) {
		return ALREADY_MAPD;
	}

	pte[tbl_idx] = ((uintptr_t)physaddr & PAGE_MASK) | pg_flags;

	return SUCCESS;
}

int pgd_unmap(uintptr_t virtaddr){
	uint32_t dir_idx = virtaddr >> 22;
	uint32_t tbl_idx = (virtaddr >> 12) & 0x3FF;

	pgd_t* pde = PGD_VADDR;
	if (!(pde[dir_idx] & _PAGE_P)) {
		return ALREADY_UMAPD;
	}

	pte_t* pte = PT_VADDR(dir_idx);
	if (!(pte[tbl_idx] & _PAGE_P)) {
		return ALREADY_UMAPD;
	}

	pte[tbl_idx] = 0;
	invlpg((void*)virtaddr);

	for (int i = 0; i < PTE_MAX_ENTRIES; i++) {
		if (pte[i] & _PAGE_P) {
			return SUCCESS;
		}
	}

	kfree_phys((void*)(pde[dir_idx] & PAGE_MASK));
	pde[dir_idx] = 0;

	invlpg((void*)(dir_idx << 22));

	return SUCCESS;
}

int pte_update_flags(uintptr_t virtaddr, mem_flags_t flags){
	uint32_t dir_idx = virtaddr >> 22;
	uint32_t tbl_idx = (virtaddr >> 12) & 0x3FF;

	pgd_t* pde = PGD_VADDR;
	if (!(pde[dir_idx] & _PAGE_P)) {
		return INVALID_ARG;
	}

	pte_t* pte = PT_VADDR(dir_idx);
	if (!(pte[tbl_idx] & _PAGE_P)) {
		return INVALID_ARG;
	}

	uintptr_t physaddr = pte[tbl_idx] & PAGE_MASK;

	uint8_t pg_flags = _PAGE_P;
	if(flags & MEM_WRITE)   pg_flags |= _PAGE_RW;
	if(flags & MEM_USER)    pg_flags |= _PAGE_US;

	pte[tbl_idx] = physaddr | pg_flags;
	invlpg((void*)virtaddr);

	return SUCCESS;
}

mem_flags_t pte_get_flags(uintptr_t virtaddr){
	uint32_t dir_idx = virtaddr >> 22;
	uint32_t tbl_idx = (virtaddr >> 12) & 0x3FF;

	pgd_t* pde = PGD_VADDR;
	if (!(pde[dir_idx] & _PAGE_P)) {
		return 0;
	}

	pte_t* pte = PT_VADDR(dir_idx);
	if (!(pte[tbl_idx] & _PAGE_P)) {
		return 0;
	}

	mem_flags_t flags = 0;

	flags |= MEM_READ;

	if (pte[tbl_idx] & _PAGE_RW) {
		flags |= MEM_WRITE;
	}
	
	if (pte[tbl_idx] & _PAGE_US) {
		flags |= MEM_USER;
	}else{
		flags |= MEM_KERNEL;
	}

	return flags;
}

void* pgd_translate(void* virtaddr){
	uint32_t dir_idx = (uintptr_t)virtaddr >> 22;
	uint32_t tbl_idx = ((uintptr_t)virtaddr >> 12) & 0x3FF;
	uint32_t offset  = (uintptr_t)virtaddr & 0xFFF;

	pgd_t* pde = PGD_VADDR;
	if (!(pde[dir_idx] & _PAGE_P)) {
		return NULL;
	}

	pte_t* pte = PT_VADDR(dir_idx);
	if (!(pte[tbl_idx] & _PAGE_P)) {
		return NULL;
	}

	uintptr_t physaddr = (pte[tbl_idx] & PAGE_MASK) | offset;
	return (void*)physaddr;
}

int pgd_dup_current(pgd_t* dest, uint8_t copy_kernel_area){

	pgd_t* src = PGD_VADDR;

	for (int i = 0; i < PGD_MAX_PTE; i++){
		if(i == SELF_PDE_INDEX){
			continue;
		}

		if (!(src[i] & _PAGE_P)) {
			dest[i] = 0;
			continue;
		}

		if (i >= (KERNEL_VIRT_BASE >> 22) && !copy_kernel_area) {
			dest[i] = src[i];
			continue;
		}

		if (src[i] & _PAGE_P) {			
			pte_t* src_pt = PT_VADDR(i);
			pte_t* dest_pt = kcalloc(PTE_MAX_ENTRIES, sizeof(pte_t));
			if (!dest_pt) {
				for(int j = 0; j < i; j++){
					if(dest[j] & _PAGE_P){
						kfree_phys((void*)(dest[j] & PAGE_MASK));
					}
				}

				return NO_MEMORY;
			}

			for (int j = 0; j < PTE_MAX_ENTRIES; j++) {
				dest_pt[j] = src_pt[j];
			}

			dest[i] = (pgd_t)pgd_translate(dest_pt) | (src[i] & FLAGS_MASK);
		}
	}

	return SUCCESS;
}

void pgd_copy(pgd_t* dest, pgd_t* src){
	for (int i = 0; i < PGD_MAX_PTE; i++) {
		if(i == SELF_PDE_INDEX) continue;
		dest[i] = src[i];
	}
}

void pgd_copy_kernel(pgd_t* kernel_pgd, pgd_t* dest){
	for (int i = (KERNEL_VIRT_BASE >> 22); i < PGD_MAX_PTE; i++) {
		if(i == SELF_PDE_INDEX) continue;
		dest[i] = kernel_pgd[i];
	}
}

void pgd_remove_kernel(pgd_t* dest){
	for (int i = (KERNEL_VIRT_BASE >> 22); i < PGD_MAX_PTE; i++) {
		if(i == SELF_PDE_INDEX) continue;
		dest[i] = 0;
	}
}
