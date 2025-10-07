#include <wey/elf.h>
#include <wey/binfmts.h>
#include <wey/vfs.h>
#include <wey/mmu.h>
#include <wey/sched.h>
#include <def/err.h>

static int load_elf_binarie(struct binprm *bprm);

struct binfmt elf_format = {
    .load_binary = load_elf_binarie
};

static int _validade_elf_ehdr(struct Elf32_Ehdr* ehdr) {
    if (!ehdr) {
        return NULL_PTR;
    }

    if(memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        return INVALID_FORMAT; // Not an ELF file
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32 || ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        return NOT_SUPPORTED; // Only 32-bit ELF files are supported
    }

    if (ehdr->e_machine != EM_386) {
        return NOT_SUPPORTED; // Only x86 architecture is supported
    }
    
    if (ehdr->e_type != ET_EXEC) {
        return INVALID_ARG; // Not an executable ELF file
    }

    if (ehdr->e_version != EV_CURRENT || ehdr->e_ident[EI_VERSION] != EV_CURRENT) {
        return NOT_SUPPORTED; // Unsupported ELF version
    }

    return OK;
}

static int load_elf_binarie(struct binprm *bprm){
	if(!bprm->file || !bprm->filename){
		return NULL_PTR;
	}

	int res = SUCCESS;

	vfs_lseek(bprm->file, 0, SEEK_SET);
	res = vfs_read(bprm->file, bprm->buff, sizeof(bprm->buff));

	if(IS_STAT_ERR(res)){
        return res;
    }

	struct Elf32_Ehdr* ehdr = (struct Elf32_Ehdr*)bprm->buff;
	struct Elf32_Phdr* phdrs = (struct Elf32_Phdr*)((uint8_t*)ehdr + ehdr->e_phoff);

	if((res = _validade_elf_ehdr(ehdr)) != OK){
		return INVALID_FORMAT;
	}

	uint8_t flags = FPAGING_P | FPAGING_US;

	for (int i = 0; i < ehdr->e_phnum; i++) {
		struct Elf32_Phdr* phdr = &phdrs[i];
		if (phdr->p_type != PT_LOAD){
			continue;
		}

		void* segment = kmalloc(phdr->p_memsz);
		if(!segment){
			return NO_MEMORY;
		}

		vfs_lseek(bprm->file, phdr->p_offset, SEEK_SET);
		int readBytes = vfs_read(bprm->file, segment, phdr->p_filesz);

		if(IS_STAT_ERR(readBytes) || readBytes != phdr->p_filesz){
			kfree(segment);
			return READ_FAIL;
		}

		// Zero extra part (BSS)
		memset(segment + phdr->p_filesz, 0x0, phdr->p_memsz - phdr->p_filesz);

		res = vma_add(bprm->mm, 
			(void*)phdr->p_vaddr, 
			segment, 
			phdr->p_memsz, 
			flags | (phdr->p_flags & PF_W ? FPAGING_RW : 0),
			1
		);

		mmu_map_pages(
			(void*)phdr->p_vaddr,
			mmu_translate(segment),
			phdr->p_memsz,
			flags | (phdr->p_flags & PF_W ? FPAGING_RW : 0)
		);

		if(res != SUCCESS){
			return res;
		}
	}

	bprm->entryPoint = (void*)ehdr->e_entry;

	return SUCCESS;
}
