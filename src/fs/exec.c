#include <fs/binfmts.h>
#include <fs/vfs.h>
#include <mmu.h>
#include <uaccess.h>
#include <core/sched.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/err.h>

extern struct binfmt elf_format;
extern struct binfmt script_format;

static struct binfmt* fmts[] = {
    &elf_format, // ELF binary format
    &script_format, // Script binary format
    0x0
};

static struct binprm* alloc_binprm(int fd, const char* filename) {
    return ERR_PTR(NO_MEMORY);
}

static void destroy_binprm(struct binprm* bprm, uint8_t free_mm){
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
    return NOT_IMPLEMENTED;
}
