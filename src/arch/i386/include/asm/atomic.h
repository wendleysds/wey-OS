#ifndef _X86_ATOMIC_H
#define _X86_ATOMIC_H

#include <def/compile.h>

typedef struct {
	volatile int value;
} atomic_t;

static __always_inline int arch_atomic_fetch_add(atomic_t* v, int inc) {
	__asm__ volatile(
		"lock xaddl %0, %1"
		: "+r"(inc), "+m"(v->value)
		:
		: "memory"
	);
	return inc;
}

static __always_inline int arch_atomic_read(const atomic_t* v) {
	int val;
	__asm__ volatile(
		"movl %1, %0"
		: "=r"(val)
		: "m"(v->value)
		: "memory"
	);
	return val;
}

static __always_inline void arch_atomic_set(atomic_t* v, int val) {
	__asm__ volatile(
		"movl %1, %0"
		: "+m"(v->value)
		: "r"(val)
		: "memory"
	);
}

static __always_inline int arch_atomic_cmpxchg(atomic_t* v, int expected, int new) {
	int old;
	__asm__ volatile(
		"lock cmpxchgl %2, %1"
		: "=a"(old), "+m"(v->value)
		: "r"(new), "0"(expected)
		: "memory"
	);
	return old;
}

static __always_inline void arch_atomic_add(int i, atomic_t *v){
	__asm__ volatile(
		"lock addl %1, %0"
		: "+m" (v->value)
		: "ir" (i)
		: "memory"
	);
}

static __always_inline void arch_atomic_sub(int i, atomic_t *v){
	__asm__ volatile(
		"lock subl %1, %0"
		: "+m" (v->value)
		: "ir" (i)
		: "memory"
	);
}

static __always_inline void arch_atomic_inc(atomic_t *v){
	__asm__ volatile(
		"lock incl %0"
		: "+m" (v->value)
		:
		: "memory"
	);
}

static __always_inline void arch_atomic_dec(atomic_t *v){
	__asm__ volatile(
		"lock decl %0"
		: "+m"
		(v->value)
		:
		: "memory"
	);
}

static __always_inline int arch_atomic_dec_and_test(atomic_t *v){
	char result;
	__asm__ volatile(
		"lock decl %0; setz %1"
		: "+m"(v->value), "=q"(result)
		:
		: "memory"
	);
	return result;
}

#endif