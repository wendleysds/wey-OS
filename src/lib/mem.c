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
