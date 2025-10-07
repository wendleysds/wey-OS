#ifndef _STRING_H
#define _STRING_H

void* memset(void*, int, unsigned long);
void* memcpy(void* dest, const void* src, unsigned long length);
int memcmp(const void *s1, const void *s2, int count);
void* memmove(void *dest, const void *src, unsigned long n);

int strlen(const char *s);
int strnlen(const char *s, int maxlen);
char *strcpy(char *restrict dst, const char *restrict src);
char *strncpy(char *restrict dst, const char *restrict src, int maxlen);
char *strcat(char *restrict dst, const char *restrict src);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strbrk(const char *s, const char *accept);
char *strtok(char *restrict str, const char *restrict delim);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int maxlen);
char* strdup(const char *s);

#endif
