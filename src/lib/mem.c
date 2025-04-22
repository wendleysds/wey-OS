#include <lib/mem.h>

void* memset(void* ptr, int c, unsigned long size){
	char* temp = (char*) ptr;

	for(unsigned long i = 0; i < size; i++){
		temp[i] = (char) c;
	}

	return ptr;
}

void* memcpy(void* dest, void* src, unsigned long length){
	char* d = dest;
	char* s = src;

	while(length--){
		*d++ = *s++;
	}

	return dest;
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

