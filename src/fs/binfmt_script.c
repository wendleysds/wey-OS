#include <wey/binfmts.h>
#include <def/status.h>

static int load_script(struct binprm *bprm);

struct binfmt script_format = {
    .load_binary = load_script
};

static int load_script(struct binprm *bprm){
    return NOT_IMPLEMENTED;
}