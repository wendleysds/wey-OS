#ifndef _X86_DIV64_H
#define _X86_DIV64_H

static inline uint32_t __do_div_arch(uint64_t *n, uint32_t base){
	uint32_t low = (uint32_t)(*n);
	uint32_t high = (uint32_t)(*n >> 32);
	uint32_t rem;

	asm (
		"divl %4"
		: "=a"(low), "=d"(rem)
		: "a"(low), "d"(high), "r"(base)
	);

	*n = low;
	return rem;
}

#define do_div(n, base) __do_div_arch(&(n), base)

#endif
