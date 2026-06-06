#ifndef _DIV64_H
#define _DIV64_H

#include <asm/div64.h>

uint32_t __do_div_generic(uint64_t *n, uint32_t base);

#ifndef do_div
#define do_div(n, base) __do_div_generic(&(n), (base))
#endif

#endif
