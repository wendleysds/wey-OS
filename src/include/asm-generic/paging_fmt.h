#ifndef _GENERIC_PAGING_FMT_H
#define _GENERIC_PAGING_FMT_H

#include <stdint.h>

#define MAX_LEVELS 4

struct paging_level {
	uint8_t shift;
	uint64_t mask;
};

struct paging_size {
	uint8_t level;
	uint8_t buddy_order;
	size_t size;
	uint32_t flag;
};

struct paging_format {
	uint8_t levels;
	const struct paging_level lvl[MAX_LEVELS];

	uint8_t nr_sizes;
	const struct paging_size sizes[MAX_LEVELS];
};

#endif
