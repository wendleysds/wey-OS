#include <exec/binfmts.h>
#include <def/status.h>
#include <kernel/init.h>

static int load_script(struct binprm *bprm){
    return NOT_IMPLEMENTED;
}

static struct binfmt script_format = {
    .load_binary = load_script
};

static int __init _register_scriptfmt(){
	binfmt_register(&script_format);
	return SUCCESS;
}

core_initcall(_register_scriptfmt);
