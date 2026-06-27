#ifndef _ERRNO_H
#define _ERRNO_H

#include <def/compile.h>
#include <uapi/sys/errno.h>

#define MAX_ERRNO 255

#define IS_ERR_VALUE(x) unlikely((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)

static inline void __must_check *ERR_PTR(long error){
	return (void*)error;
}

static inline long __must_check PTR_ERR(const void *ptr){
	return (long)ptr;
}

static inline int __must_check IS_ERR(const void *ptr){
	return IS_ERR_VALUE((unsigned long)ptr);
}

static inline int __must_check IS_ERR_OR_NULL(const void *ptr){
	return unlikely(!ptr) || IS_ERR_VALUE((unsigned long)ptr);
}

static inline long __must_check PTR_ERR_OR_ZERO(const void *ptr){
	if (IS_ERR(ptr)) return PTR_ERR(ptr);
	else return 0;
}

static inline void* __must_check ERR_CAST(const void *ptr){
	return (void*)ptr;
}

#define OK 0
#define SUCCESS OK

#endif