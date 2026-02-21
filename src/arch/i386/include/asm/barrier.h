#ifndef _X86_BARRIER_H
#define _X86_BARRIER_H

#include <def/compile.h>

static __always_inline void arch_mb(){
	asm volatile("mfence":::"memory");
}

static __always_inline void arch_rmb(){
	asm volatile("lfence":::"memory");
}

static __always_inline void arch_wmb(){
	asm volatile("sfence" ::: "memory");
}

#endif
