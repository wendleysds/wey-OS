#include <wey/mmu.h>
#include <wey/vfs.h>
#include <wey/vma.h>
#include <wey/panic.h>
#include <def/err.h>
#include <def/init.h>
#include <asm/page.h>

/**
 * Main virtual memory management unit (MMU) kernel API
 */

static pgd_t* _kernel_pgd = NULL;

int __init mmu_init(){
	_kernel_pgd = pgd_alloc();
	if(!_kernel_pgd){
		return NO_MEMORY;
	}

	int res = pgd_dup_current(_kernel_pgd, 1);
	if(IS_ERR_VALUE(res)){
		goto out;
	}

	if(IS_ERR_VALUE(res = pgd_load(_kernel_pgd))){
		goto out;
	}

	extern uintptr_t __kernel_text_start;
	extern uintptr_t __kernel_text_end;

	res = mmu_set_flags(
		&__kernel_text_start,
		(size_t)&__kernel_text_end - (size_t)&__kernel_text_start,
		(MEM_READ | MEM_KERNEL)
	);

	if(IS_ERR_VALUE(res)){
		goto out;
	}

	extern uintptr_t __kernel_rodata_start;
	extern uintptr_t __kernel_rodata_end;

	res = mmu_set_flags(
		&__kernel_rodata_start,
		(size_t)&__kernel_rodata_end - (size_t)&__kernel_rodata_start,
		(MEM_READ | MEM_KERNEL)
	);

	if(IS_ERR_VALUE(res)){
		goto out;
	}

	extern uintptr_t __boot_end;

	res = mmu_munmap(
		0x0,
		(size_t)&__boot_end
	);

out:
	return res;
}

int mmu_mmap(void* physaddr, void* virtaddr, int size, mem_flags_t mem_flags){
	if(!physaddr || !virtaddr || size <= 0){
		return INVALID_ARG;
	}

	uintptr_t paddr = (uintptr_t)physaddr;
	uintptr_t vaddr = (uintptr_t)virtaddr;

	unsigned int alignedSize = (size + PTE_PAGE_SIZE - 1) & ~(PTE_PAGE_SIZE - 1);
	unsigned int count = alignedSize / PTE_PAGE_SIZE;

	int pflags = mmu_flags_arch(mem_flags);
	for(unsigned int i = 0; i < count; i++){
		int status = pgd_map(vaddr, paddr, pflags);
		if(status != SUCCESS){
			vaddr = (uintptr_t)virtaddr;
			for(unsigned int j = 0; j < i; j++){
				if(pgd_unmap(vaddr) != SUCCESS){
					panic("mmu_mmap(): failed to rollback map!\n");
				}

				vaddr += PTE_PAGE_SIZE;
			}

			return status;
		}

		vaddr += PTE_PAGE_SIZE;
		paddr += PTE_PAGE_SIZE;
	}

	return SUCCESS;
}

int mmu_munmap(void* virtaddr, int size){
	if(size <= 0){
		return INVALID_ARG;
	}

	uintptr_t vaddr = (uintptr_t)virtaddr;

	unsigned int alignedSize = (size + PTE_PAGE_SIZE - 1) & ~(PTE_PAGE_SIZE - 1);
	unsigned int count = alignedSize / PTE_PAGE_SIZE;

	for(unsigned int i = 0; i < count; i++){
		int status = pgd_unmap(vaddr);
		if(status != SUCCESS){
			return status;
		}

		vaddr += PTE_PAGE_SIZE;
	}

	return SUCCESS;
}

int mmu_set_flags(void* virtaddr, int size, mem_flags_t flags){
	if(!virtaddr || size <= 0){
		return INVALID_ARG;
	}

	uintptr_t vaddr = (uintptr_t)virtaddr;

	unsigned int alignedSize = (size + PTE_PAGE_SIZE - 1) & ~(PTE_PAGE_SIZE - 1);
	unsigned int count = alignedSize / PTE_PAGE_SIZE;

	int target_flags = mmu_flags_arch(flags);
	int old_flags = pte_get_flags(vaddr);

	for(unsigned int i = 0; i < count; i++){
		int status = pte_update_flags(vaddr, target_flags);
		if(status != SUCCESS){
			vaddr = (uintptr_t)virtaddr;
			for(unsigned int j = 0; j < i; j++){
				if(pte_update_flags(vaddr, old_flags) != SUCCESS)
					panic("mmu_set_flags(): failed to rollback flags!\n");

				vaddr += PTE_PAGE_SIZE;
			}

			return status;
		}

		vaddr += PTE_PAGE_SIZE;
	}

	return SUCCESS;
}

int mmu_page_switch(pgd_t* pgd){
	return pgd_load(pgd);
}

int mmu_destroy_page(pgd_t* pgd){
	int res = pgd_load(_kernel_pgd);
	if(IS_STAT_ERR(res)){
		return res;
	}

	pgd_remove_kernel(pgd);

	pgd_free(pgd);

	return SUCCESS;
}

pgd_t* mmu_create_page(){
	pgd_t* pgd = pgd_alloc();
	if(pgd){
		pgd_copy_kernel(_kernel_pgd, pgd);
	}

	return pgd;
}

void* mmu_translate(void* virtaddr){
	return pgd_translate(virtaddr);
}
