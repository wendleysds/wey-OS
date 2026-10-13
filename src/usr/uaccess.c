#include <kernel/uaccess.h>	
#include <asm-generic/extable.h>
#include <asm/usr/access.h>
#include <def/linker.h>
#include <def/compile.h>
#include <def/errno.h>
#include <stddef.h>
/*
 * @brief Copy kernel data to userspace
 * @param dst Address in userspace where the data will be copied
 * @param src Address in kernel where the data will be copied from
 * @param len Length of the data to be copied
 * @return number of bytes remaining to copy
 */
int copy_to_user(void __user *dst, const void *src, size_t len){
	if(!dst || !src){
		return -EINVAL;
	}

	if(len == 0) return 0;

	return raw_copy_to_user(dst, src, len);;
}

/*
 * @brief Copy userspace data to kernel
 * @param dst Address in kernel where the data will be copied
 * @param src Address in userspace where the data will be copied from
 * @param len Length of the data to be copied
 * @return number of bytes remaining to copy 
 */
int copy_from_user(void *dst, const void __user *src, size_t len){
	if(!dst || !src){
		return -EINVAL;
	}

	if(len == 0) return 0;

	return raw_copy_from_user(dst, src, len);	
}
