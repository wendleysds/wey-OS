#include <mmu.h>
#include <memory/paging.h>
#include <def/err.h>
#include <core/kernel.h>
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

int mmu_init(struct PagingDirectory** kernelDirectory){
	struct PagingDirectory* dir;
	_currentDirectory = 0x0;

	uint32_t tableAmount = PAGING_TOTAL_ENTRIES_PER_TABLE;
	dir = paging_new_directory(
		tableAmount, 
		(FPAGING_P | FPAGING_RW), 
		(FPAGING_P | FPAGING_RW)
	);

	if(!dir || dir->tableCount != tableAmount){
		return NO_MEMORY;
	}

	*kernelDirectory = dir;

	paging_switch(dir);
	enable_paging();

	idt_register_callback(14, &_page_fault_handler);

	return SUCCESS;
}

struct PagingDirectory* mmu_create_page(){
	return paging_new_directory_empty();
}

int mmu_page_switch(struct PagingDirectory* directory){
	if(!directory){
		return NULL_PTR;
	}

	paging_switch(directory);

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

void* mmu_translate(struct PagingDirectory* directory, void* virt){
	return paging_translate(directory, virt);
}

uint8_t mmu_user_pointer_valid(void* ptr){
	if(!ptr){
		return 0;
	}
	
	return paging_is_user_pointer_valid(ptr);
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

struct mem_region* vma_lookup(struct mm_struct* mm, void* virtualAddr){
	if(!mm || !virtualAddr){
		return 0x0;
	}

	struct mem_region* region = mm->regions;
	while(region){
		if(virtualAddr >= region->virtualBaseAddress && virtualAddr < region->virtualEndAddress){
			return region;
		}

		region = region->next;
	}

	return 0x0;
}

int vma_add(struct mm_struct* mm, void* virtualAddr, void* physicalAddr, uint32_t size, uint8_t flags, uint8_t isPrivate){
	if (!mm || !mm->pageDirectory) {
		return NULL_PTR;
	}

	struct mem_region* region = (struct mem_region*)kzalloc(sizeof(struct mem_region));
	if (!region) {
		return NO_MEMORY;
	}

	region->physBaseAddress = physicalAddr;

	region->virtualBaseAddress = virtualAddr;
	region->virtualEndAddress = paging_align_address((void*)((uintptr_t)virtualAddr + size));

	region->flags = flags;
	region->size = size;
	region->isPrivite = isPrivate;

	region->next = mm->regions;
	mm->regions = region;

	return SUCCESS;
}

int vma_remove(struct mm_struct* mm, void* virtualAddr, uint32_t size){
	if (!mm || !mm->pageDirectory) {
		return NULL_PTR;
	}

	struct mem_region* prev = NULL;
	struct mem_region* current = mm->regions;

	while (current) {
		if (current->virtualBaseAddress == virtualAddr && current->size == size) {
			if (prev) {
				prev->next = current->next;
			} else {
				mm->regions = current->next;
			}

			if(current->physBaseAddress){
				kfree(current->physBaseAddress);
			}

			kfree(current);
			break;
		}
		prev = current;
		current = current->next;
	}

	return SUCCESS;
}

int vma_clean(struct mm_struct* mm){
	if (!mm) {
		return NULL_PTR;
	}

	struct mem_region* current = mm->regions;
	while (current) {
		struct mem_region* next = current->next;

		if(current->isPrivite){
			if(current->virtualBaseAddress && current->size > 0){
				mmu_unmap_pages(mm->pageDirectory, current->virtualBaseAddress, current->size);
			}

			if(current->physBaseAddress){
				kfree(current->physBaseAddress);
			}

			kfree(current);
		}
		
		current = next;
	}

	return SUCCESS;
}

int vma_destroy(struct mm_struct* mm){
	if (!mm) {
		return NULL_PTR;
	}

	vma_clean(mm);

	if (mm->pageDirectory) {
		mmu_destroy_page(mm->pageDirectory);
	}

	kfree(mm);

	return SUCCESS;
}
