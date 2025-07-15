#include <fs/binfmts.h>
#include <def/status.h>

static struct binfmt elf_format = {
    .load_binary = load_script
};

static int load_script(struct binprm *bprm){
    return NOT_IMPLEMENTED;
}