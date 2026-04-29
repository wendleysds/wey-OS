#include <wey/mmu.h>
#include <wey/vfs.h>
#include <wey/vma.h>
#include <wey/panic.h>
#include <def/err.h>
#include <def/init.h>
#include <def/config.h>
#include <asm/page.h>

/**
 * Main virtual memory management unit (MMU) kernel API
 */

pgd_t* _kernel_pgd = NULL;

int __init mmu_init(){
	return NOT_IMPLEMENTED;
}

int mmu_mmap(void* physaddr, void* virtaddr, int size, mem_flags_t mem_flags){
	return NOT_IMPLEMENTED;
}

int mmu_munmap(void* virtaddr, int size){
	return NOT_IMPLEMENTED;
}

int mmu_set_flags(void* virtaddr, int size, mem_flags_t flags){
	return NOT_IMPLEMENTED;
}

int mmu_page_switch(pgd_t* pgd){
	return NOT_IMPLEMENTED;
}

int mmu_destroy_page(pgd_t* pgd){
	return NOT_IMPLEMENTED;
}

pgd_t* mmu_create_page(){
	return ERR_PTR(NOT_IMPLEMENTED);
}

void* mmu_translate(void* virtaddr){
	return ERR_PTR(NOT_IMPLEMENTED);
}
