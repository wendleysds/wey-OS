#ifndef _GENERIC_PAGING_CTX_H
#define _GENERIC_PAGING_CTX_H

#include "paging_fmt.h"
#include "paging_ops.h"

struct paging_ctx {
	void *root; // pgd
	const struct paging_format* fmt;
	const struct paging_ops *ops;
};

#endif