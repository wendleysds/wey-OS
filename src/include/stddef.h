#ifndef _STDDEF_H
#define _STDDEF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#ifndef __cplusplus
typedef __WCHAR_TYPE__ wchar_t;
#endif

#ifdef __cplusplus
	#define NULL nullptr
#else
	#define NULL ((void*)0)
#endif

#define offsetof(type, member) \
	__builtin_offsetof(type, member)

typedef struct {
	long long __ll;
	long double __ld;
} max_align_t;

#ifdef __cplusplus
}
#endif

#endif
