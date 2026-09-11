#ifndef _KERNEL_KMAP_H
#define _KERNEL_KMAP_H

struct page;

void *kmap(struct page *page);
void kunmap(struct page *page);

#endif