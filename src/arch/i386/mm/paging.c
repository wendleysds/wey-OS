#include <stdint.h>
#include <wey/panic.h>
#include <mm/page.h>
#include <wey/vma.h>
#include <asm/page.h>
#include <lib/string.h>
#include <def/err.h>
#include <def/config.h>

#define ADDRS_NOT_ALING(virt, phys) \
	(((uintptr_t)(virt) & (PAGE_SIZE - 1)) || \
	((uintptr_t)(phys) & (PAGE_SIZE - 1)))

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

pgd_t* pgd_current(){
	pgd_t* ret;
	__asm__ volatile ("mov %%cr3, %0" : "=rm" (ret));
	return (pgd_t*)__va(ret);
}

int pgd_load(pgd_t* pgd){
	if(!pgd){
		return NULL_PTR;
	}

	pgd_t* pgd_cur = pgd_current();

	if(pgd_cur == pgd){
		return SUCCESS;
	}

	if ((uintptr_t)(pgd) & (PAGE_SIZE - 1)) {
		return BAD_ALIGNMENT;
	}

	__asm__ volatile (
		"mov %0, %%cr3"
		:
		: "r"(pgd_translate(pgd))
		: "memory"
	);

	return SUCCESS;
}

static int pgd_direct_map_page(struct page* page, int flags){
	return pgd_map(page_to_virt(page),page_to_phys(page), flags);
}

static int pgd_direct_unmap_page(struct page* page){
	return pgd_unmap(page_to_virt(page));
}

pgd_t* pgd_alloc(){
	struct page* pgd_page = page_alloc(1, PAGE_KERNEL | PAGE_TABLE);
	pgd_t* pgd = NULL;

	if(!pgd_page){
		return NULL;
	}

	if(pgd_direct_map_page(pgd_page, (_PAGE_P | _PAGE_RW)) != SUCCESS){
		page_free(pgd_page);
		return NULL;
	}

	pgd = (pgd_t*)page_to_virt(pgd_page);
	pgd[SELF_PDE_INDEX].val = (uintptr_t)page_to_phys(pgd_page) | _PAGE_P | _PAGE_RW;

	return pgd;
}

void pgd_free(pgd_t* pgd){
	pgd_t* pgd_cur = pgd_current();

	if(pgd_cur == pgd){
		panic("pgd_free(): Trying to free current directory!");
	}

	struct page* p = NULL;

	pgd[SELF_PDE_INDEX].val = 0;

	for (int i = 0; i < PGD_MAX_ENTRIES; i++) {
		if (pgd[i].val & _PAGE_P) {
			p = phys_to_page(pgd[i].val & PAGE_MASK);
			page_free(p);
		}
	}

	p = virt_to_page((uintptr_t)pgd);
	page_free(p);
}

static int _map(uintptr_t virtaddr, uintptr_t physaddr, int flags){
	uint32_t dir_idx = virtaddr >> 22;
	uint32_t tbl_idx = (virtaddr >> 12) & 0x3FF;

	pgd_t* pde = PGD_VADDR;
	pte_t* pte = PT_VADDR(dir_idx);

	if (!(pde[dir_idx].val & _PAGE_P)) {
		struct page* pt_page = page_alloc(1, PAGE_KERNEL | PAGE_TABLE);
		if(!pt_page){
			return NO_MEMORY;
		}

		pde[dir_idx].val = (uintptr_t)page_to_phys(pt_page) | flags | _PAGE_P;
		pte = PT_VADDR(dir_idx);
		memset(pte, 0x0, PAGE_SIZE);
	}

	pte[tbl_idx].val = ((uintptr_t)physaddr & PAGE_MASK) | flags;

	return SUCCESS;
}

int pgd_map(uintptr_t virtaddr, uintptr_t physaddr, int flags){
	if(ADDRS_NOT_ALING(virtaddr, physaddr)){
		return BAD_ALIGNMENT;
	}

	uint32_t dir_idx = virtaddr >> 22;
	uint32_t tbl_idx = (virtaddr >> 12) & 0x3FF;

	pgd_t* pde = PGD_VADDR;
	pte_t* pte = PT_VADDR(dir_idx);

	if ((pde[dir_idx].val & _PAGE_P) && (pte[tbl_idx].val & _PAGE_P)) {
		return ALREADY_MAPD;
	}

	return _map(virtaddr, physaddr, flags);
}

int pgd_remap(uintptr_t virtaddr, uintptr_t physaddr, int flags){
	if(ADDRS_NOT_ALING(virtaddr, physaddr)){
		return BAD_ALIGNMENT;
	}

	return _map(virtaddr, physaddr, flags);
}

int pgd_unmap(uintptr_t virtaddr){
	uint32_t dir_idx = virtaddr >> 22;
	uint32_t tbl_idx = (virtaddr >> 12) & 0x3FF;

	pgd_t* pde = PGD_VADDR;
	if (!(pde[dir_idx].val & _PAGE_P)) {
		return ALREADY_UMAPD;
	}

	pte_t* pte = PT_VADDR(dir_idx);
	if (!(pte[tbl_idx].val & _PAGE_P)) {
		return ALREADY_UMAPD;
	}

	struct page* p = phys_to_page(pde[dir_idx].val & PAGE_MASK);
	if(!p) return INVALID_ARG;

	pte[tbl_idx].val = 0;
	invlpg((void*)virtaddr);

	for (int i = 0; i < PTE_MAX_ENTRIES; i++) {
		if (pte[i].val & _PAGE_P) {
			return SUCCESS;
		}
	}

	int status = pgd_direct_unmap_page(p);
	if(IS_ERR_VALUE(status)){
		return status;
	}

	status = page_free(p);
	if(IS_ERR_VALUE(status)){
		pgd_direct_map_page(p, pde[dir_idx].val & FLAGS_MASK);
		return status;
	}

	pde[dir_idx].val = 0;

	invlpg((void*)(dir_idx << 22));

	return SUCCESS;
}

int pte_update_flags(uintptr_t virtaddr, int flags){
	uint32_t dir_idx = virtaddr >> 22;
	uint32_t tbl_idx = (virtaddr >> 12) & 0x3FF;

	pgd_t* pde = PGD_VADDR;
	if (!(pde[dir_idx].val & _PAGE_P)) {
		return INVALID_ARG;
	}

	pte_t* pte = PT_VADDR(dir_idx);
	if (!(pte[tbl_idx].val & _PAGE_P)) {
		return INVALID_ARG;
	}

	uintptr_t physaddr = pte[tbl_idx].val & PAGE_MASK;

	pte[tbl_idx].val = physaddr | flags;
	invlpg((void*)virtaddr);

	return SUCCESS;
}

int pte_get_flags(uintptr_t virtaddr){
	uint32_t dir_idx = virtaddr >> 22;
	uint32_t tbl_idx = (virtaddr >> 12) & 0x3FF;

	pgd_t* pde = PGD_VADDR;
	if (!(pde[dir_idx].val & _PAGE_P)) {
		return 0;
	}

	pte_t* pte = PT_VADDR(dir_idx);
	if (!(pte[tbl_idx].val & _PAGE_P)) {
		return 0;
	}

	return pte[tbl_idx].val & FLAGS_MASK;
}

void* pgd_translate(void* virtaddr){
	uint32_t dir_idx = (uintptr_t)virtaddr >> 22;
	uint32_t tbl_idx = ((uintptr_t)virtaddr >> 12) & 0x3FF;
	uint32_t offset  = (uintptr_t)virtaddr & 0xFFF;

	pgd_t* pde = PGD_VADDR;
	if (!(pde[dir_idx].val & _PAGE_P)) {
		return NULL;
	}

	pte_t* pte = PT_VADDR(dir_idx);
	if (!(pte[tbl_idx].val & _PAGE_P)) {
		return NULL;
	}

	uintptr_t physaddr = (pte[tbl_idx].val & PAGE_MASK) | offset;
	return (void*)physaddr;
}

int pgd_dup_current(pgd_t* pgd, uint8_t copy_kernel_area){
	pgd_t* src = PGD_VADDR;
	struct page* p = NULL;

	for (int i = 0; i < PGD_MAX_ENTRIES; i++){
		if(i == SELF_PDE_INDEX){
			continue;
		}

		if (!(src[i].val & _PAGE_P)) {
			pgd[i].val = 0;
			continue;
		}

		if (i >= (PAGE_OFFSET >> 22) && !copy_kernel_area) {
			pgd[i] = src[i];
			continue;
		}

		if (src[i].val & _PAGE_P) {			
			pte_t* src_pt = PT_VADDR(i);
			p = page_alloc(1, PAGE_KERNEL | PAGE_TABLE);
			if (!p) {
				for(int j = 0; j < i; j++){
					if(pgd[j].val & _PAGE_P){
						p = phys_to_page(pgd[j].val & PAGE_MASK);
						page_free(p);
					}
				}
				
				return NO_MEMORY;
			}
			
			pte_t* dest_pt = (pte_t*)page_to_virt(p);
			pgd_direct_map_page(p, src[i].val & FLAGS_MASK);

			for (int j = 0; j < PTE_MAX_ENTRIES; j++) {
				dest_pt[j] = src_pt[j];
			}

			pgd[i].val = (uintptr_t)page_to_phys(p) | (src[i].val & FLAGS_MASK);
		}
	}

	return SUCCESS;
}

void pgd_copy(pgd_t* dest, pgd_t* src){
	for (int i = 0; i < PGD_MAX_ENTRIES; i++) {
		if(i == SELF_PDE_INDEX) continue;
		dest[i] = src[i];
	}
}

void pgd_copy_kernel(pgd_t* kernel_pgd, pgd_t* pgd){
	for (int i = (PAGE_OFFSET >> 22); i < PGD_MAX_ENTRIES; i++) {
		if(i == SELF_PDE_INDEX) continue;
		pgd[i] = kernel_pgd[i];
	}
}

void pgd_remove_kernel(pgd_t* pgd){
	for (int i = (PAGE_OFFSET >> 22); i < PGD_MAX_ENTRIES; i++) {
		if(i == SELF_PDE_INDEX) continue;
		pgd[i].val = 0;
	}
}

int mmu_flags_arch(mem_flags_t flags){
	int f = 0;

	if(flags & MEM_READ)
		f |= _PAGE_P;
	if(flags & MEM_WRITE)
		f |= _PAGE_RW;
	if(flags & MEM_USER)
		f |= _PAGE_US;

	if (flags & MEM_GLOBAL) 
		f |= _PAGE_GLOBAL;

	if (flags & MEM_DEVICE)
		f |= _PAGE_PCD | _PAGE_PWT;
	
	return f;
}

mem_flags_t arch_mmu_flags(int flags){
	mem_flags_t f = 0;

	if(flags & _PAGE_P)
		f |= MEM_READ;
	if(flags & _PAGE_RW)
		f |= MEM_WRITE;
	if(flags & _PAGE_US)
		f |= MEM_USER;

	if (flags & _PAGE_GLOBAL) 
		f |= MEM_GLOBAL;

	return f;
}
