#ifndef _PAGE_H
#define _PAGE_H

extern char __page_offset[];
#define __va(addr) ((uintptr_t)(addr) + (uintptr_t)__page_offset)
#define __pa(addr) ((uintptr_t)(addr) - (uintptr_t)__page_offset)

#endif
