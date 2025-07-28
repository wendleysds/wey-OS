#include <fs/binfmts.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <mmu.h>
#include <uaccess.h>
#include <core/sched.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/status.h>

extern struct binfmt elf_format;
extern struct binfmt script_format;

static struct binfmt* fmts[] = {
    &elf_format, // ELF binary format
    &script_format, // Script binary format
    0x0
};

static struct binprm* alloc_binprm(int fd, const char* filename) {
    struct binprm* bprm = (struct binprm*)kmalloc(sizeof(struct binprm));
    if (!bprm) {
        return 0x0;
    }

    struct mm_struct* mm = (struct mm_struct*)kmalloc(sizeof(struct mm_struct));
    if(!mm){
        kfree(bprm);
        return 0x0;
    }

    struct PagingDirectory* dir = mmu_create_page();
    if(!dir){
        kfree(bprm);
        kfree(mm);
        return 0x0;
    }

    char buffer[PATH_MAX];
    if(copy_string_from_user(buffer, filename, sizeof(buffer)) != SUCCESS){
        goto err;
    }

    bprm->filename = strdup(buffer);
    if(!bprm->filename){
        goto err;
    }

    mm->pageDirectory = dir;
    mm->regions = NULL;

    bprm->mm = mm;
    bprm->entryPoint = NULL;
    bprm->loadAddress = NULL;

    bprm->fd = fd;
    bprm->argc = 0;
    bprm->envc = 0;

    return bprm;

err:
    kfree(bprm);
    kfree(mm);
    mmu_destroy_page(dir);
    return 0x0;
}

static void destroy_binprm(struct binprm* bprm, uint8_t free_mm){
    if(free_mm){
        if(bprm->mm->regions){
            struct mem_region* region = bprm->mm->regions;

            while(region){
                struct mem_region* next = region->next;
                kfree(region);
                region = next;
            }
        }

        mmu_destroy_page(bprm->mm->pageDirectory);
        kfree(bprm->mm);
    }

    kfree(bprm->filename);
    close(bprm->fd);
    kfree(bprm);
}

static int exec_binprm(struct binprm* bprm) {
    if (!bprm || !bprm->filename || bprm->fd < 2) {
        return INVALID_ARG; // Invalid binary parameters
    }

    int res = SUCCESS;
    struct binfmt* fmt;
    for (int i = 0; (fmt = fmts[i]); i++){
        if((res = fmt->load_binary(bprm)) == SUCCESS){
            break;
        }

        if(res == NO_MEMORY){
            return res;
        }
    }

    if(res != SUCCESS){
        return INVALID_FILE;
    }

    return SUCCESS;
}

int exec(const char* pathname, const char* argv[], const char* envp[]) {
    if(!pathname || !argv || !envp || strlen(pathname) == 0 || strlen(pathname) > PATH_MAX) {
        return INVALID_ARG;
    }

    int fd = open(pathname, O_RDONLY);
    if (fd < 0) {
        return fd;
    }

    struct binprm* bprm = alloc_binprm(fd, pathname);
    if (!bprm) {
        close(fd);
        return NO_MEMORY; // Failed to allocate binprm
    }

    // Copy argv and envp to the process stack

    int8_t status;
    if((status = exec_binprm(bprm)) != SUCCESS){
        destroy_binprm(bprm, 1);
        close(fd);
        return status;
    }

    return status;
}
