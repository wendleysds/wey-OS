#ifndef _ERROR_H
#define _ERROR_H

#include <stdint.h>
#include <def/compile.h>

#define MAX_ERRNO 4095

#define IS_ERR_VALUE(x) unlikely((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)

static inline void __must_check *ERR_PTR(long error){
	return (void *) error;
}

static inline long __must_check PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static inline uint8_t __must_check IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

static inline uint8_t __must_check IS_ERR_OR_NULL(const void *ptr)
{
	return unlikely(!ptr) || IS_ERR_VALUE((unsigned long)ptr);
}

static inline long __must_check PTR_ERR_OR_ZERO(const void *ptr)
{
	if (IS_ERR(ptr))
		return PTR_ERR(ptr);
	else
		return 0;
}

#endif