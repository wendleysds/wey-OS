#ifndef _MATH_INTERNAL_H
#define _MATH_INTERNAL_H

#include <stdint.h>

uint64_t udivmod64(uint64_t a, uint64_t b, uint64_t *rem);
uint32_t do_div_32(uint64_t *n, uint32_t base);
uint64_t do_div_64(uint64_t *n, uint64_t base);

#endif /* _MATH_INTERNAL_H */
