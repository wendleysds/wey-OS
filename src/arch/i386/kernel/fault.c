#include <wey/sched.h>
#include <wey/vma.h>
#include <wey/vfs.h>
#include <wey/panic.h>
#include <wey/printk.h>
#include <wey/interrupt.h>
#include <def/err.h>

#include <mm/kheap.h>
#include <mm/page.h>

extern void page_fault_entry();

typedef struct {
	unsigned long addr;
	unsigned long err;
	int8_t present, write, user, rsvd, exec;
} pf_info_t;

static inline pf_info_t pf_decode(unsigned long err, unsigned long cr2){
	pf_info_t pf;

	pf.addr = cr2;
	pf.err = err;
	pf.present = err >> 0 & 1;
	pf.write   = err >> 1 & 1;
	pf.user    = err >> 2 & 1;
	pf.rsvd    = err >> 3 & 1;
	pf.exec    = err >> 4 & 1;

	return pf;
}

static inline unsigned long cr2(){
	unsigned long cr2;
	__asm__ volatile("mov %%cr2, %0" : "=r" (cr2));
	return cr2;
}

static inline void dump_regs(struct registers* regs){
	printk(
		"eax %#010lX ebx %#010lX ecx %#010lX edx %#010lX\n",
		regs->ax, regs->bx, regs->cx, regs->dx
	);

	printk( 
		"esi %#010lX edi %#010lX esp %#010lX ebp %#010lX\n",
		regs->si, regs->di, regs->sp, regs->bp
	);

	printk(
		"ip %#010lX cs %#010lX ss %#010lX\n",
		regs->ip, regs->cs, regs->ss
	);

	printk(
		"eflags %#010lX kesp %#010lX\n",
		regs->flags, regs->ksp
	);

	printk(
		"int %#010lX err %#010lX\n",
		regs->int_no, regs->err_code
	);
}

static inline void show_pf_info(pf_info_t* pf){
	printk(
		"\nPage fault at %#010lx (ec=%#010lx) [present=%d, write=%d, user=%d, rsvd=%d, exec=%d]\n", 
		pf->addr, pf->err, pf->present, pf->write, pf->user, pf->rsvd, pf->exec
	);
}

static int vm_handle_file(struct vm_region* region, uintptr_t addr){
	uintptr_t page_addr = addr & ~(PAGE_SIZE - 1);

	size_t region_offset = page_addr - region->start;
	off_t file_offset = region->file_offset + region_offset;

	struct page* page = page_alloc(1, 0x0);
	if (!page) return NO_MEMORY;

	int res = mmu_mmap(
		current->mm->ctx,
		page_to_phys(page),
		page_addr,
		PAGE_SIZE,
		region->mem_flags
	);

	if(IS_ERR_VALUE(res)){
		page_free(page);
		return res;
	}

	memset((void*)page_addr, 0x0, PAGE_SIZE);

	if (region->file) {
		vfs_lseek(region->file, file_offset, SEEK_SET);

		int n = vfs_read(region->file, (void*)page_addr, PAGE_SIZE);
		if (n < 0){
			page_free(page);
			return n;
		}
	}

	return SUCCESS;
}

static int vm_handle_stack(struct vm_region* region, uintptr_t addr){
	uintptr_t page_addr = addr & ~(PAGE_SIZE - 1);
	struct page* page = page_alloc(1, 0x0);
	if (!page) return NO_MEMORY;

	int res = mmu_mmap(
		current->mm->ctx,
		page_to_phys(page),
		page_addr,
		PAGE_SIZE,
		region->mem_flags
	);

	if(IS_ERR_VALUE(res)){
		page_free(page);
		return res;
	}

	memset((void*)page_addr, 0x0, PAGE_SIZE);

	return SUCCESS;
}

static int vm_handle_cow(struct vm_region* region, uintptr_t addr){
	uintptr_t page_addr = addr & ~(PAGE_SIZE - 1);

	if (page_addr < region->start || page_addr >= region->end){
		return ACCESS_DENIED;
	}

	uintptr_t phys = mmu_translate(
		current->mm->ctx,
		addr
	);

	struct page* page = phys_to_page(phys);
	if (!page) return NOT_FOUND;

	if(atomic_read(&page->refcount) == 1){
		mem_flags_t flags = mmu_get_flags(
			current->mm->ctx,
			page_addr
		);

		mmu_set_flags(
			current->mm->ctx,
			page_addr,
			flags | MEM_WRITE
		);
	
		return SUCCESS;
	}

	struct page* new_page = page_alloc(1, 0x0);
	if (!new_page) return NOT_FOUND;

	int res = mmu_mmap(
		current->mm->ctx,
		page_to_phys(new_page),
		page_addr,
		PAGE_SIZE,
		region->mem_flags
	);

	if(IS_ERR_VALUE(res)){
		page_free(page);
		return res;
	}

	mmu_invlpg(current->mm->ctx, page_addr);

	page_put(page);

	return SUCCESS;
}

void page_fault_handler(struct registers* regs){
	pf_info_t pf = pf_decode(regs->err_code, cr2());
	show_pf_info(&pf);
	int handle_res = -1;

	if(!current || !pf.user){
		dump_regs(regs);
		panic("Kernel page fault!");
	}

	struct vm_region* region = vma_lookup(current->mm, pf.addr);

	if(pf.present){
		if (!region)
			goto segfault;

		if (pf.write && !(region->mem_flags & MEM_WRITE))
			goto segfault;

		if (!pf.write && !(region->mem_flags & MEM_READ))
			goto segfault;

		if (pf.exec && !(region->mem_flags & MEM_EXEC))
			goto segfault;

		if(pf.write && (region->mem_flags & MEM_WRITE)){

			handle_res = vm_handle_cow(region, pf.addr);
			goto check_res;
		}
		
		goto segfault;
	}

	if (!region)
		goto segfault;

	if (pf.write && !(region->mem_flags & MEM_WRITE))
		goto segfault;

	if (!pf.write && !(region->mem_flags & MEM_READ))
		goto segfault;

	if (pf.exec && !(region->mem_flags & MEM_EXEC))
		goto segfault;

	if(region->mem_flags & MEM_GROWSDOWN){
		handle_res = vm_handle_stack(region, pf.addr);
	}
	else if(region->file){
		handle_res = vm_handle_file(region, pf.addr);
	}

check_res:
	if(handle_res != 0){
		printk("page_fault_handler: handler: failed with status \"%d\"!\n", handle_res);
		goto kill;
	}

	return; // OK

segfault:
	dump_regs(regs);
kill:
	panic("Killed thread\n");
	while(1) cpu_relax();
}

void __init fault_init(){
	idt_set_gate(
		0xE,
		(uint32_t)page_fault_entry,
		GDT_KERNEL_CODE,
		(IDT_PRESENT | IDT_DPL0 | IDT_TYPE_INT_GATE32)
	);
}
