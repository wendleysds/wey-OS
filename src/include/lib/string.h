#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

void* memset(void* d, int c, size_t m);
void* memcpy(void* d, const void* s, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void* memmove(void *d, const void *s, size_t n);
void *memchr(const void *s, int c, size_t n);

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t n);
char *strcpy(char *restrict d, const char *restrict s);
char *strncpy(char *restrict d, const char *restrict s, size_t n);
char *strcat(char *restrict d, const char *restrict s);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strbrk(const char *s, const char *accept);
char *strtok(char *restrict str, const char *restrict delim);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char* strdup(const char *s);

#endif
