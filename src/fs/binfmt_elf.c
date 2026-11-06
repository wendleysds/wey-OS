#include <kernel/init.h>
#include <exec/binfmts.h>
#include <exec/elf.h>
#include <mm/vma.h>
#include <def/errno.h>
#include <lib/string.h>

static int _validade_elf_ehdr(struct Elf32_Ehdr* ehdr) {
	if (!ehdr) {
		return -EINVAL;
	}

	if(memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
		return -EINVAL; // Not an ELF file
	}

	if (ehdr->e_ident[EI_CLASS] != ELFCLASS32 || ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		return -ENOTSUP; // Only 32-bit ELF files are supported
	}

	if (ehdr->e_machine != EM_386) {
		return -ENOTSUP; // Only x86 architecture is supported
	}

	if (ehdr->e_type != ET_EXEC) {
		return -EINVAL; // Not an executable ELF file
	}

	if (ehdr->e_version != EV_CURRENT || ehdr->e_ident[EI_VERSION] != EV_CURRENT) {
		return -ENOTSUP; // Unsupported ELF version
	}

	return OK;
}

static int load_elf_binarie(struct binprm *bprm){
	if(!bprm->file || !bprm->filename){
		return -EINVAL;
	}

	int res = SUCCESS;

	vfs_lseek(bprm->file, 0, SEEK_SET);
	res = vfs_read(bprm->file, bprm->buff, sizeof(bprm->buff));

	if(IS_ERR_VALUE(res)){
		return res;
	}

	struct Elf32_Ehdr* ehdr = (struct Elf32_Ehdr*)bprm->buff;
	struct Elf32_Phdr* phdrs = (struct Elf32_Phdr*)((uint8_t*)ehdr + ehdr->e_phoff);

	if(IS_ERR_VALUE(res = _validade_elf_ehdr(ehdr))){
		return res;
	}

	uintptr_t load_end = 0;

	for (int i = 0; i < ehdr->e_phnum; i++) {
		struct Elf32_Phdr* phdr = &phdrs[i];
		if (phdr->p_type != PT_LOAD){
			continue;
		}

		uint8_t flags = MEM_USER | MEM_READ;
		if (phdr->p_flags & PF_W) flags |= MEM_WRITE;
		if (phdr->p_flags & PF_X) flags |= MEM_EXEC;

		struct vm_region* region = vma_add(
			bprm->mm,
			phdr->p_vaddr,
			phdr->p_vaddr + phdr->p_memsz,
			flags,
			PROT_MAP_PRIVATE,
			bprm->file,
			phdr->p_offset
		);

		if(IS_ERR_VALUE(region)){
			mm_clean(bprm->mm);
			return PTR_ERR(region);
		}

		uintptr_t seg_end = phdr->p_vaddr + phdr->p_memsz;
		if (seg_end > load_end)
			load_end = seg_end;
	}

	bprm->entryPoint = (void*)ehdr->e_entry;

	/* Set brk_start just past the last loaded segment, page-aligned */
	uintptr_t brk_base = (load_end + 0xFFF) & ~0xFFFU;
	bprm->mm->brk_start = brk_base;
	bprm->mm->brk       = brk_base;

	return SUCCESS;
}

static struct binfmt elf_format = {
	.load_binary = load_elf_binarie,
};

static int __init _register_elffmt(){
	binfmt_register(&elf_format);
	return SUCCESS;
}

core_initcall(_register_elffmt);
