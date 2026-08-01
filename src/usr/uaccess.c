#include <kernel/uaccess.h>	
#include <asm-generic/extable.h>
#include <asm/usr/access.h>
#include <def/linker.h>
#include <def/compile.h>
#include <def/errno.h>
#include <stddef.h>

int copy_to_user(void __user *dst, const void *src, size_t len){
	if(!dst || !src){
		return -EINVAL;
	}

	if(len == 0) return 0;

	return raw_copy_to_user(dst, src, len);;
}

int copy_from_user(void *dst, const void __user *src, size_t len){
	if(!dst || !src){
		return -EINVAL;
	}

	if(len == 0) return 0;

	return raw_copy_from_user(dst, src, len);	
}
