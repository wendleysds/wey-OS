#include <loaders/elf.h>
#include <fs/binfmts.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <memory/kheap.h>
#include <lib/mem.h>
#include <def/status.h>

/*static struct binfmt elf_format = {
    .load_binary = load_elf_binarie
};*/

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

static struct Elf32_Phdr* _get_pheader(struct Elf32_Ehdr* header, int index){
    if (!header || index < 0 || index >= header->e_phnum) {
        return NULL; // Invalid header or index
    }

    if (header->e_phoff == 0) {
        return 0; // No program headers present
    }

    return (struct Elf32_Phdr*)((char*)header + header->e_phoff) + index;
}

static struct Elf32_Shdr* _get_sheader(struct Elf32_Ehdr* header, int index){
    if (!header || index < 0 || index >= header->e_shnum) {
        return NULL; // Invalid header or index
    }

    return (struct Elf32_Shdr*)((char*)header + header->e_shoff) + index;
}

static int load_elf_binarie(struct binprm *bprm){
    if(!bprm || !bprm->filename || !bprm->fdpath) {
        return INVALID_ARG;
    }

    if(bprm->fd < 2) {
        return INVALID_ARG;
    }

    int fd = bprm->fd;
    int status = SUCCESS;

    struct stat st;
    if((status = stat(bprm->fdpath, &st)) != SUCCESS) {
        return status; // Failed to get file status
    }

    bprm->mem.loadAddress = kmalloc(st.fileSize);
    if(!bprm->mem.loadAddress) {
        return NO_MEMORY; // Failed to allocate memory for binary
    }

    lseek(fd, 0, SEEK_SET); // Reset file descriptor to the beginning
    if((status = read(fd, bprm->mem.loadAddress, st.fileSize)) != st.fileSize) {
        kfree(bprm->mem.loadAddress);
        return status; // Failed to read the binary
    }

    if((status = _validade_elf_ehdr((struct Elf32_Ehdr*)bprm->mem.loadAddress)) != SUCCESS) {
        kfree(bprm->mem.loadAddress);
        return status; // Invalid ELF file
    }



    kfree(bprm->mem.loadAddress);
    return NOT_IMPLEMENTED;
}