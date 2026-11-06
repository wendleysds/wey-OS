#include <mm/kheap.h>
#include <lib/string.h>

void* memset(void* ptr, int c, size_t size){
	char* temp = (char*) ptr;

	for(size_t i = 0; i < size; i++){
		temp[i] = (char)c;
	}

	return ptr;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;

    while(n--){
        *d++ = *s++;
    }

    return dest;
}

int memcmp(const void *a, const void *b, size_t n) {
	const unsigned char *pa = a, *pb = b;
	while (n--) {
		if (*pa != *pb) return *pa - *pb;
		pa++; pb++;
	}
	return 0;
}

void* memmove(void *dest, const void *src, size_t n){
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

void* memchr(const void *s, int c, size_t n){
	const unsigned char *p = s;

	while (n--) {
		if (*p == (unsigned char)c)
			return (void *)p;
		p++;
	}

	return NULL;
}

size_t strlen(const char *s){
	const char *p;
	for (p = s; *p; ++p);
	return (p - s);
}

size_t strnlen(const char *s, size_t maxlen){
	size_t i = 0;
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

char *strncpy(char *d, const char *s, size_t n){
	size_t i = 0;
	for (; i < n && s[i]; i++) d[i] = s[i];
	for (; i < n; i++) d[i] = '\0';
	return d;
}

char* strcat(char *restrict dst, const char *restrict src){
	strcpy(dst + strlen(dst), src);
	return dst;
}

int strcmp(const char *s1, const char *s2){
	while (*s1 && *s1 == *s2) ++s1, ++s2;
	return (*s1 > *s2) - (*s2  > *s1);
}

int strncmp(const char *a, const char *b, size_t n){
	while (n && *a && *a == *b) { a++; b++; n--; }
	return n ? (unsigned char)*a - (unsigned char)*b : 0;
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
	char *copy;
	size_t len;

	if (!s || *s == '\0') {
		copy = (char *)kmalloc(1);
		if (!copy) return NULL;
		*copy = '\0';
		return copy; 
	}
	
	if (*s == '\0') return NULL;

	len = strlen(s);
	copy = (char *)kmalloc(len + 1);
	if (!copy) return NULL;

	memcpy(copy, s, len);
	copy[len] = '\0';
	return copy;
}

