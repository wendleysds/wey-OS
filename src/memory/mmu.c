#include "def/status.h"
#include <mmu.h>
#include <memory/paging.h>
#include <def/config.h>
#include <def/err.h>
#include <core/kernel.h>
#include <stddef.h>
#include <stdint.h>
#include <arch/i386/idt.h>
#include <drivers/terminal.h>
#include <core/sched.h>

#define _ADDRS_NOT_ALING(virt, phys) \
	(((uintptr_t)(virt) & (PAGING_PAGE_SIZE - 1)) || \
	((uintptr_t)(phys) & (PAGING_PAGE_SIZE - 1)))

struct PagingDirectory* _currentDirectory = 0x0;

static inline int _read_cr2(){
	uint32_t cr2;
	__asm__ volatile("mov %%cr2, %0" : "=r" (cr2));
	return cr2;
}

typedef struct {
	void* addr;
	uint32_t ec;
	int8_t present, write, user, rsvd, exec;
} pf_info_t;

static inline pf_info_t pf_decode(uint32_t ec, void* cr2){
	pf_info_t pf;
	memset(&pf, 0x0, sizeof(pf));

	pf.addr = cr2;
	pf.ec = ec;
	pf.present = ec >> 0 & 1;
	pf.write   = ec >> 1 & 1;
	pf.user    = ec >> 2 & 1;
	pf.rsvd    = ec >> 3 & 1;
	pf.exec    = ec >> 4 & 1;

	return pf;
}

static inline void _show_pf_info(pf_info_t pf){
	terminal_write(
		"\nPage fault at 0x%x (ec=0x%x) [present=%d, write=%d, user=%d, rsvd=%d, exec=%d]\n", 
		pf.addr, pf.ec, pf.present, pf.write, pf.user, pf.rsvd, pf.exec
	);
}

static void _page_fault_handler(struct InterruptFrame* frame){
	void* faultingAddress = (void*)frame->eip;
	pf_info_t pf = pf_decode(frame->err_code, faultingAddress);

	struct mm_struct* mm = 0x0;
	struct Task* task = pcb_current();

	if(task && task->process){
		mm = task->process->mm;
	}else{
		panic("Page fault in kernel mode!");
	}
	
	_show_pf_info(pf);

	struct mem_region* v = vma_lookup(mm, faultingAddress);

	if(!v){
		// Invalid memory access
		panic(
			"Invalid memory access at 0x%x (ec=0x%x) [present=%d, write=%d, user=%d, rsvd=%d, exec=%d] in task %d (%s)\n", 
			faultingAddress, pf.ec, pf.present, pf.write, pf.user, pf.rsvd, pf.exec,
			task->tid, task->process ? task->process->name : "unknown"
		);
	}

	if(!pf.present){
		panic(
			"Protection fault at 0x%x (ec=0x%x) [present=%d, write=%d, user=%d, rsvd=%d, exec=%d] in task %d (%s)\n", 
			faultingAddress, pf.ec, pf.present, pf.write, pf.user, pf.rsvd, pf.exec,
			task->tid, task->process ? task->process->name : "unknown"
		);
	}

	// kill process
	
	while(1){
		__asm__ volatile ("hlt");
	}
}

int mmu_init(){
	struct PagingDirectory* dir = mmu_create_page();

	extern uintptr_t __kernel_phys_start;
	extern uintptr_t __kernel_phys_end;
	extern uintptr_t __kernel_high_start;

	size_t kernel_size = (size_t)&__kernel_phys_end - (size_t)&__kernel_phys_start;

	uint8_t flags = (FPAGING_P | FPAGING_RW);

	int res = mmu_map_pages(
		dir,
		(void*)&__kernel_high_start, 
		(void*)&__kernel_phys_start, 
		kernel_size,
		flags
	);

	if(IS_STAT_ERR(res)){
		return res;
	}

	res = mmu_map_pages(
		dir,
		(void*)KERNEL_STACK_VIRT_BOTTOM,
		(void*)KERNEL_STACK_PHYS_BOTTOM,
		KERNEL_STACK_SIZE,
		flags
	);

	if(IS_STAT_ERR(res)){
		return res;
	}

	res = mmu_map_pages(
		dir,
		(void*)HEAP_VIRT_BASE,
		(void*)HEAP_PHYS_BASE,
		HEAP_SIZE_BYTES,
		flags
	);

	if(IS_STAT_ERR(res)){
		return res;
	}

	res = mmu_map_pages(
		dir, 
		(void*)HEAP_TABLE_VIRT_BASE, 
		(void*)HEAP_TABLE_PHYS_BASE, 
		(HEAP_SIZE_BYTES / HEAP_BLOCK_SIZE), 
		flags
	);

	if(IS_STAT_ERR(res)){
		return res;
	}

	extern struct VideoStructPtr* _get_video();
	struct VideoStructPtr* vPtr = _get_video();

	res = mmu_map_pages(
		dir,
		(void*)KERNEL_FB_VIRT_BASE,
		(void*)vPtr->framebuffer_physical,
		(vPtr->height * vPtr->width * (vPtr->bpp / 8)),
		flags
	);

	if(IS_STAT_ERR(res)){
		return res;
	}

	dir->entry[1023] = (PagingTable)((uintptr_t)mmu_translate(dir->entry) | FPAGING_P | FPAGING_RW); // self-referencing PDE
	
	idt_register_callback(14, &_page_fault_handler);

	return mmu_page_switch(dir);
}

struct PagingDirectory* mmu_create_page(){
	return paging_new_directory();
}

int mmu_page_switch(struct PagingDirectory* directory){
	if(!directory){
		return NULL_PTR;
	}

	if (_currentDirectory == directory) {
		return SUCCESS;
	}

	if (!directory->entry) {
		return NULL_PTR;
	}

	if ((uintptr_t)(directory->entry) & (PAGING_PAGE_SIZE - 1)) {
		return BAD_ALIGNMENT;
	}

	if (directory->tableCount == 0) {
		return INVALID_ARG;
	}

	if (directory->tableCount > PAGING_TOTAL_ENTRIES_PER_TABLE) {
		return OUT_OF_BOUNDS;
	}

	uint32_t* physAddr = (uint32_t*)mmu_translate(directory->entry);

	paging_load_directory(physAddr);
	_currentDirectory = directory;

	return SUCCESS;
}

int mmu_destroy_page(struct PagingDirectory* directory){
	if(!directory){
		return NULL_PTR;
	}

	paging_free_directory(directory);

	return SUCCESS;
}

int mmu_map_pages(struct PagingDirectory* directory, void* virtualAddr, void* physicalAddr, uint32_t size, uint8_t flags){
	uint32_t alignedSize = (size + PAGING_PAGE_SIZE - 1) & ~(PAGING_PAGE_SIZE - 1);
	uint32_t count = alignedSize / PAGING_PAGE_SIZE;

	return paging_map_range(directory, count, paging_align_to_lower(virtualAddr),  paging_align_to_lower(physicalAddr), flags);
}

int mmu_unmap_pages(struct PagingDirectory* directory, void* virtualStart, uint32_t size){
	uint32_t alignedSize = (size + PAGING_PAGE_SIZE - 1) & ~(PAGING_PAGE_SIZE - 1);
	uint32_t count = alignedSize / PAGING_PAGE_SIZE;

	return paging_unmap_range(directory, count, paging_align_to_lower(virtualStart));
}

void* mmu_translate(void* virtualAddr){
	uintptr_t virt = (uintptr_t)virtualAddr;
	uint32_t dir_idx = virt >> 22;
	uint32_t tbl_idx = (virt >> 12) & 0x3FF;

	uint32_t pde = VIRT_PDIR[dir_idx];
	if (!(pde & FPAGING_P)) {
		return 0;
	}

	uint32_t* pt = VIRT_PTBL(dir_idx);
	uint32_t pte = pt[tbl_idx];
	if (!(pte & FPAGING_P)) {
		return 0;
	}

	uintptr_t phys = (pte & 0xFFFFF000) | (virt & 0xFFF);
	return (void*)phys;
}

uint8_t mmu_user_pointer_valid(void* ptr){
	if(!ptr){
		return 0;
	}
	
	return 0;
}

uint8_t mmu_user_pointer_valid_range(const void* userPtr, size_t size){
    uintptr_t start = (uintptr_t)userPtr;
    uintptr_t end = start + size;

    if (end < start){
        return 0;
	}

    for (uintptr_t addr = start; addr < end; addr += PAGING_PAGE_SIZE){
        if (!mmu_user_pointer_valid((void*)addr)){
            return 0;
		}
    }

    return 1;
}

void* phys_to_virt(void* physicalAddr){
	return (void*)((uintptr_t)physicalAddr + KERNEL_VIRT_BASE - KERNEL_PHYS_BASE);
}
