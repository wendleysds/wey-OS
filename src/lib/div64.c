#include <lib/div64.h>

uint32_t __do_div_generic(uint64_t *n, uint32_t base){
	uint64_t dividend = *n;
	uint64_t quotient = 0;
	uint64_t remainder = 0;

	for (int i = 63; i >= 0; i--) {
		remainder <<= 1;
		remainder |= (dividend >> i) & 1;

		if (remainder >= base) {
			remainder -= base;
			quotient |= (1ULL << i);
		}
	}

	*n = quotient;
	return (uint32_t)remainder;
}
