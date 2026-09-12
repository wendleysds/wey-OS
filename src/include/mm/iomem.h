#ifndef _IOMEM_H
#define _IOMEM_H

#include <sys/types.h>
#include <io/mmio.h>

void __iomem *ioremap(paddr_t phys, size_t size);
void iounmap(void __iomem *virt);

#endif