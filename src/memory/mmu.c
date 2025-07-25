#include <mmu.h>
#include <core/kernel.h>

void mmu_init(struct PagingDirectory** kernelDirectory){
	struct PagingDirectory* dir;

	uint32_t tableAmount = PAGING_TOTAL_ENTRIES_PER_TABLE;
	dir = paging_new_directory(tableAmount, FPAGING_RW | FPAGING_P);

	if(!dir || dir->tableCount != tableAmount){
		panic("Failed to initializing paging!");
	}

	*kernelDirectory = dir;

	paging_switch(dir);
	enable_paging();
}