#include <loaders/elf.h>
#include <fs/binfmts.h>
#include <fs/fs.h>
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
    if(!bprm || !bprm->filename) {
        return INVALID_ARG;
    }

    if(bprm->fd < 2) {
        return INVALID_ARG;
    }

    int fd = bprm->fd;
    int status = SUCCESS;

    struct stat st;
    if((status = stat(bprm->filename, &st)) != SUCCESS) {
        return status; // Failed to get file status
    }

    bprm->loadAddress = kmalloc(st.fileSize);
    void* buffer = bprm->loadAddress;

    if(!buffer) {
        return NO_MEMORY; // Failed to allocate memory for binary
    }

    lseek(fd, 0, SEEK_SET); // Reset file descriptor to the beginning
    if((status = read(fd, buffer, st.fileSize)) != st.fileSize) {
        kfree(buffer);
        return status; // Failed to read the binary
    }

    struct Elf32_Ehdr* ehdr = (struct Elf32_Ehdr*)buffer;

    if((status = _validade_elf_ehdr(ehdr)) != SUCCESS) {
        kfree(buffer);
        return status; // Invalid ELF file
    }

    bprm->entryPoint = (void*)ehdr->e_entry;

    struct Elf32_Phdr* phdrs = (struct Elf32_Phdr*)((uint8_t*)buffer + ehdr->e_phoff);

    void** phys_segments = kmalloc(sizeof(void*) * phdrs->p_memsz);
    if (!phys_segments) {
        kfree(buffer);
        return NO_MEMORY;
    }

    int alloc_counter = 0;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        struct Elf32_Phdr* phdr = &phdrs[i];
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        phys_segments[alloc_counter] = kmalloc(phdr->p_memsz);
        if (!phys_segments[alloc_counter]) {
            for(int i = 0; i < alloc_counter; i++){
                kfree(phys_segments[i]);
            }

            kfree(phys_segments);
            kfree(buffer);
            return NO_MEMORY;
        }

        void* phys_segment = phys_segments[alloc_counter++];

        memcpy(phys_segment, buffer + phdr->p_offset, phdr->p_filesz);
        memset((char*)phys_segment + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);

        uint8_t flags = FPAGING_P | FPAGING_US;
        if (phdr->p_flags & PF_W) {
            flags |= FPAGING_RW;
        }

        void* addr = (void*)phdr->p_vaddr;
        uint32_t len = phdr->p_memsz;

        status = mmu_map_pages(
            bprm->mm->pageDirectory,
            addr,
            phys_segment,
            len,
            flags
        );

        struct mem_region* region = (struct mem_region*)kmalloc(sizeof(struct mem_region));
        if (!region || status != SUCCESS) {
            if(region){
                kfree(region);
            }

            for (int j = 0; j < alloc_counter; j++) {
                kfree(phys_segments[j]);
            }

            kfree(phys_segments);
            kfree(buffer);
            return NO_MEMORY;
        }

        region->virtualBaseAddress = addr;
        region->virtualEndAddress  = addr + len;
        region->flags = flags;

        region->next = bprm->mm->regions;
        bprm->mm->regions = region;
    }

    kfree(phys_segments);
    kfree(buffer);
    return SUCCESS;
}
