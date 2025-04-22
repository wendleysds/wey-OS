#ifndef _MEMORY_H
#define _MEMORY_H

void* memset(void*, int, unsigned long);
void* memcpy(void* dest, void* src, unsigned long length);
int memcmp(const void *s1, const void *s2, int count);

#endif
