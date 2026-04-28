#ifndef _GENERIC_PAGING_FMT_H
#define _GENERIC_PAGING_FMT_H

#include <stdint.h>

#define MAX_LEVELS 4

struct paging_level {
	uint8_t shift;
	uint64_t mask;
};

struct paging_format {
	uint8_t levels;
	struct paging_level lvl[MAX_LEVELS];
	uint64_t page_size;
};

#endif
