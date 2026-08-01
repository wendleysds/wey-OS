#ifndef _X86_USER_ACCESS_H
#define _X86_USER_ACCESS_H

#include <kernel/uaccess.h>
#include <def/compile.h>

extern asmlinkage int raw_copy_from_user(void* to, const void __user* from, size_t size);
extern asmlinkage int raw_copy_to_user(void __user* to, const void* from, size_t size);

#endif