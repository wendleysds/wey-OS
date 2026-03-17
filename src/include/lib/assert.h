#ifndef _ASSERTS_H
#define _ASSERTS_H

#include <def/compile.h>
#include <wey/printk.h>
#include <wey/panic.h>

__no_return void __assert_fail(
	const char* expr,
	const char* file,
	int line,
	const char* func
);

#ifdef NDEBUG
	#define ASSERT(expr) ((void)0)
	#define ASSERT_MSG(expr, msg, ...) ((void)0)
#else
	#define ASSERT(expr) \
		(likely(expr) ? (void)0 : __assert_fail(#expr, __FILE__, __LINE__, __func__))

	#define ASSERT_MSG(expr, msg, ...) do { \
		if (unlikely(!(expr))) { \
			printk("ASSERTION FAILED: " msg "\n", ##__VA_ARGS__); \
			__assert_fail(#expr, __FILE__, __LINE__, __func__); \
		} \
	} while(0)
#endif

#define ASSERT_STATIC(expr, msg) _Static_assert(expr, msg)

#define BUG() panic("BUG at %s:%d in %s()", __FILE__, __LINE__, __func__)

#define BUG_ON(cond) do { \
	if (unlikely(cond)) BUG(); \
} while(0)

#define BUG_MSG(cond, msg, ...) do { \
	if (unlikely(cond)) { \
		printk("BUG: " msg "\n", ##__VA_ARGS__); \
		BUG(); \
	} \
} while(0)

#define WARN_ON(cond) ({ \
	int __ret = !!(cond); \
	if (unlikely(__ret)) { \
		printk("WARNING at %s:%d in %s()\n", __FILE__, __LINE__, __func__); \
	} \
	__ret; \
})

#define WARN_ONCE(cond) ({ \
	static bool __warned = false; \
	int __ret = !!(cond); \
	if (unlikely(__ret && !__warned)) { \
		__warned = true; \
		printk("WARNING (ONCE) at %s:%d in %s()\n", __FILE__, __LINE__, __func__); \
	} \
	__ret; \
})

#define unreachable() \
	__assert_fail("unreachable", __FILE__, __LINE__, __func__); \
	__builtin_unreachable()

#endif