#include <wey/mmu.h>
#include <wey/vfs.h>
#include <wey/vma.h>
#include <wey/panic.h>
#include <def/err.h>

pgd_t* _kernel_pgd = NULL;

int mmu_init(){
	_kernel_pgd = pgd_alloc();
	if(!_kernel_pgd){
		return NO_MEMORY;
	}

	int res = pgd_dup_current(_kernel_pgd, 1);
	if(IS_STAT_ERR(res)){
		panic("mmu_init(): pgd dup failed! %d", res);
	}

	res = pgd_load(_kernel_pgd);
	if(IS_STAT_ERR(res)){
		panic("mmu_init(): pgd kernel load failed! %d", res);
	}

	extern uintptr_t __boot_end;

	res = mmu_munmap(
		NULL, 
		(void*)0x8000, 
		((size_t)&__boot_end - 0x8000)
	);

	if(IS_STAT_ERR(res)){
		panic("mmu_init(): Failed to unmap boot area! %d", res);
	}

	extern uintptr_t __kernel_rodata_start;
	extern uintptr_t __kernel_rodata_end;

	res = mmu_set_flags(
		NULL,
		&__kernel_rodata_start,
		(size_t)&__kernel_rodata_end - (size_t)&__kernel_rodata_start,
		(MEM_READ | MEM_KERNEL)
	);

	return res;
}

int mmu_mmap(struct mm_struct* mm, void* physaddr, void* virtaddr, int size, mem_flags_t mem_flags, uint8_t map_flags){
	if(!physaddr || !virtaddr || size <= 0){
		return INVALID_ARG;
	}

	uintptr_t paddr = (uintptr_t)physaddr;
	uintptr_t vaddr = (uintptr_t)virtaddr;

	unsigned int alignedSize = (size + PTE_PAGE_SIZE - 1) & ~(PTE_PAGE_SIZE - 1);
	unsigned int count = alignedSize / PTE_PAGE_SIZE;

	struct mem_region* mem_region = NULL;
	if(mm){
		mem_region = vma_add(mm, physaddr, virtaddr, size, mem_flags, map_flags);
		if(!mem_region){
			return NO_MEMORY;
		}
	}

	for(unsigned int i = 0; i < count; i++){
		int status = pgd_map(paddr, vaddr, mem_flags);
		if(status != SUCCESS){
			vaddr = (uintptr_t)virtaddr;
			for(unsigned int j = 0; j < i; j++){
				if(pgd_unmap(vaddr) != SUCCESS){
					warning("mmu_mmap(): failed to rollback map!\n");
				}

				vaddr += PTE_PAGE_SIZE;
			}

			if(mm){
				vma_remove(mm, mem_region, virtaddr, size);
				kfree(mem_region);
			}

			return status;
		}

		vaddr += PTE_PAGE_SIZE;
		paddr += PTE_PAGE_SIZE;
	}

	return SUCCESS;
}

int mmu_munmap(struct mm_struct* mm, void* virtaddr, int size){
	if(!virtaddr || size <= 0){
		return INVALID_ARG;
	}

	uintptr_t vaddr = (uintptr_t)virtaddr;

	struct mem_region* mem_region = NULL;

	if(mm){
		mem_region = vma_lookup(mm, virtaddr);
		if(IS_ERR(mem_region)){
			return PTR_ERR(mem_region);
		}

		if(mem_region->size > size){
			return OUT_OF_BOUNDS;
		}
	}

	unsigned int alignedSize = (size + PTE_PAGE_SIZE - 1) & ~(PTE_PAGE_SIZE - 1);
	unsigned int count = alignedSize / PTE_PAGE_SIZE;

	for(unsigned int i = 0; i < count; i++){
		int status = pgd_unmap(vaddr);
		if(status != SUCCESS){
			return status;
		}

		vaddr += PTE_PAGE_SIZE;
	}

	if(mm){
		vma_remove(mm, mem_region, virtaddr, size);
	}

	return SUCCESS;
}

int mmu_set_flags(struct mm_struct* mm, void* virtaddr, int size, mem_flags_t flags){
	if(!virtaddr || size <= 0){
		return INVALID_ARG;
	}

	uintptr_t vaddr = (uintptr_t)virtaddr;
	mem_flags_t old_flags;

	unsigned int alignedSize = (size + PTE_PAGE_SIZE - 1) & ~(PTE_PAGE_SIZE - 1);
	unsigned int count = alignedSize / PTE_PAGE_SIZE;

	struct mem_region* mem_region = NULL;

	if(mm){
		mem_region = vma_lookup(mm, virtaddr);
		if(IS_ERR(mem_region)){
			return PTR_ERR(mem_region);
		}

		old_flags = mem_region->mem_flags;
	}else{
		old_flags = pte_get_flags(vaddr);
	}

	for(unsigned int i = 0; i < count; i++){
		int status = pte_update_flags(vaddr, flags);
		if(status != SUCCESS){
			vaddr = (uintptr_t)virtaddr;
			for(unsigned int j = 0; j < i; j++){
				if(pte_update_flags(vaddr, old_flags) != SUCCESS)
					warning("mmu_set_flags(): failed to rollback flags!\n");

				vaddr += PTE_PAGE_SIZE;
			}

			return status;
		}

		vaddr += PTE_PAGE_SIZE;
	}

	if(mm){
		mem_region->mem_flags = flags;
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
