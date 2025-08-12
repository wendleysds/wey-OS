#include <loaders/elf.h>
#include <fs/binfmts.h>
#include <fs/vfs.h>
#include <mmu.h>
#include <core/sched.h>
#include <def/status.h>

static int load_elf_binarie(struct binprm *bprm);

struct binfmt elf_format = {
    .load_binary = load_elf_binarie
};

static int _validade_elf_ehdr(struct Elf32_Ehdr* ehdr) {
    if (!ehdr) {
        return INVALID_ARG; // Invalid ELF header
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

    return SUCCESS;
}

static int load_elf_binarie(struct binprm *bprm){
    return NOT_IMPLEMENTED;
}
