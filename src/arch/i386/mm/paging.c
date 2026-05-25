#include <asm-generic/paging_ctx.h>
#include <wey/vma.h>
#include <asm/page.h>
#include <def/config.h>

#include <stdint.h>

#define ADDR_NOT_ALING(addr) ((uintptr_t)(addr) & (PAGE_SIZE - 1))

static void x86_invlpg(uintptr_t virt){
	__asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

static void x86_invlpg_all(void){
	__asm__ volatile(
		"mov %%cr3, %%eax\n\r"
		"mov %%eax, %%cr3\n\r"
		::: "memory"
	);
}

static pte_t x86_mk_pte(uintptr_t phys, uint32_t flags) {
	return (pte_t){ .val = (phys & PAGE_MASK) | (flags & FLAGS_MASK) | _PAGE_P };
}

static uintptr_t x86_pte_phys(pte_t p) {
	return p.val & PAGE_MASK;
}

static uint32_t x86_pte_flags(pte_t p) {
	return p.val & FLAGS_MASK;
}

static int x86_present(pte_t p) {
	return p.val & _PAGE_P;
}

static int x86_leaf(pte_t p, uint8_t level) {
	return (p.val & _PAGE_PSIZE) || level == 1;
}

static void x86_set(pte_t *dst, pte_t v) {
	*dst = v;
}

static void x86_clear(pte_t *dst) {
	dst->val = 0;
}

static void* x86_to_virt(pte_t p) {
	return (void*)__va(x86_pte_phys(p));
}

static pte_t x86_mk_table(uintptr_t phys, uint8_t user) {
	uint8_t flags = _PAGE_P | _PAGE_RW;
	if(user) flags |= _PAGE_US;
	return (pte_t){ .val = (phys & PAGE_MASK) | (flags & FLAGS_MASK)};
}

const struct paging_ops arch_paging_ops = {
	.mk_pte = x86_mk_pte,
	.pte_phys = x86_pte_phys,
	.pte_flags = x86_pte_flags,
	.pte_present = x86_present,
	.pte_leaf = x86_leaf,
	.set_pte = x86_set,
	.clear_pte = x86_clear,
	.pte_to_virt = x86_to_virt,
	.mk_table = x86_mk_table,
	.flush_tlb_one = x86_invlpg,
	.flush_all = x86_invlpg_all,
};

const struct paging_format arch_paging_fmt = {
	.levels = 2,
	.lvl = {
		{ .shift = 22, .mask = 0x3FF },
		{ .shift = 12, .mask = 0x3FF }
	},
	.page_size = PAGE_SIZE
};

int mmu_flags_arch(mem_flags_t flags){
	int f = _PAGE_P;

	if(flags & MEM_WRITE)
		f |= _PAGE_RW;

	if(flags & MEM_USER)
		f |= _PAGE_US;

	if(flags & MEM_GLOBAL)
		f |= _PAGE_GLOBAL;

	if(flags & MEM_HUGE_PAGE)
		f |= _PAGE_PSIZE;

	if(flags & MEM_DEVICE)
		f |= _PAGE_PCD | _PAGE_PWT;

	return f;
}

mem_flags_t arch_mmu_flags(int flags){
	mem_flags_t f = 0;

	if(flags & _PAGE_P)
		f |= MEM_READ;

	if(flags & _PAGE_RW)
		f |= MEM_WRITE;

	if (flags & (_PAGE_PCD | _PAGE_PWT))
		f |= MEM_DEVICE;

	if (flags & _PAGE_GLOBAL) 
		f |= MEM_GLOBAL;

	if(flags & _PAGE_PSIZE)
		f |= MEM_HUGE_PAGE;

	if(flags & _PAGE_US)
		f |= MEM_USER;

	return f;
}
