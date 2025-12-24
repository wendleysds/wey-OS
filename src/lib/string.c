#include <lib/string.h>
#include <mm/kheap.h>

void* memset(void* ptr, int c, unsigned long size){
	char* temp = (char*) ptr;

	for(unsigned long i = 0; i < size; i++){
		temp[i] = (char) c;
	}

	return ptr;
}

void* memcpy(void* dest, const void* src, unsigned long length){
	char *d = (char*)dest;
	const char *s = (const char*)src;

	if(d < s){
		while(length--){
			*d++ = *s++;
		}
	}
	else{
		while(length--){
			*(d + length) = *(s + length);
		}
	}

	return d;
}

int memcmp(const void* s1, const void* s2, int count){
	char* c1 = (char*) s1;
	char* c2 = (char*) s2;

	while(count-- > 0){
		if(*c1++ != *c2++){
			return c1[-1] < c2[-1] ? -1 : 1;
		}
	}

	return 0;
}

void* memmove(void *dest, const void *src, unsigned long n){
	char *d = (char *)dest;
	const char *s = (const char *)src;

	if(d < s || d >= s + n) {
		// Non-overlapping regions, copy forwards
		while(n--) {
			*d++ = *s++;
		}
	} else {
		// Overlapping regions, copy backwards
		d += n;
		s += n;
		while(n--) {
			*(--d) = *(--s);
		}
	}

	return dest;
}

int strlen(const char *s){
	const char *p;
	for (p = s; *p; ++p);
	return (p - s);
}

int strnlen(const char *s, int maxlen){
	int i = 0;
	for(; i < maxlen; i++){
		if(!s[i]) break;
	}

	return i;
}

char* strcpy(char *restrict dst, const char *restrict src){
	size_t len = strlen(src);
	memcpy((void*)dst, (void*)src, len);
	dst[len] = '\0';

	return dst;
}

char* strncpy(char *restrict dst, const char *restrict src, int maxlen){
	for(int i = 0; i < maxlen - 1; i++)
	{
		if(!*src) break;

		*dst = *src;
		src++;
		dst++;
	}

	return dst;
}

char* strcat(char *restrict dst, const char *restrict src){
	strcpy(dst + sizeof(dst), src);
	return dst;
}

int strcmp(const char *s1, const char *s2){
	while (*s1 && *s1 == *s2) ++s1, ++s2;
	return (*s1 > *s2) - (*s2  > *s1);
}

int strncmp(const char *s1, const char *s2, int maxlen){
	while (maxlen && *s1 && (*s1 == *s2))
  {
		++s1;
		++s2;
		--maxlen;
	}

	if (maxlen == 0)
		return 0;
	else
		return (*(unsigned char *)s1 - *(unsigned char *)s2);
}

char *strchr(const char *s, int c){
	for (; *s != '\0' && *s != c; ++s);
	return *s == c ? (char *) s : NULL;
}

char *strrchr(const char *s, int c) {
	const char *p = NULL;
	for (;;) {
		if (*s == (char)c)
			p = s;
		if (*s++ == '\0')
			return (char *)p;
	}
}

char* strbrk(const char *s, const char *accept){
	for (; *s; ++s) {
		for (const char *a = accept; *a; ++a) {
			if (*s == *a) {
				return (char *)s;
			}
		}
	}
	return NULL;
}

char* strtok(char *restrict s, const char *restrict delim){
	static char* p = NULL;
	if(s)
		p = s;

	if(!p)
		return NULL;

	while(*p && strchr(delim, *p)){
		p++;
	}

	if(*p == '\0')
		return NULL;

	char* t = p;

	while(*p && !strchr(delim, *p)){
		p++;
	}

	if(*p){
		*p = '\0';
		p++;
	}

	return t;
}

char* strdup(const char *s) {
	if (!s) return NULL;
	if (*s == '\0') return NULL; // Handle empty string case

	long len = strlen(s);
	char *copy = (char *)kmalloc(len + 1);
	if (!copy) return NULL; // Memory allocation failed

	memcpy(copy, s, len);
	copy[len] = '\0'; // Null-terminate the string
	return copy;
}

