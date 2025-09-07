#include <stdlib.h>

void itoa(int value, char* result, int base){
	const char* digits = "0123456789abcdef";
	char buffer[32];
	int i = 0, j = 0;

	int max_result_len = sizeof(buffer);

	if (base < 2 || base > 16) {
		if (result && max_result_len > 0)
			result[0] = '\0';
		return;
	}

	if (value < 0 && base == 10){
		if (j < max_result_len - 1)
			result[j++] = '-';
		value = -value;
	}

	do {
		if (i < (int)sizeof(buffer) - 1)
			buffer[i++] = digits[value % base];
		value /= base;
	} while(value);

	while(i > 0 && j < max_result_len - 1){
		result[j++] = buffer[--i];
	}

	result[j] = '\0';
}

void utoa(unsigned int value, char* result, int base){
	const char* digits = "0123456789abcdef";
	char buffer[32];
	int i = 0, j = 0; 

	do {
		buffer[i++] = digits[value % base];
		value /= base;
	} while(value);

	while(i > 0){
		result[j++] = buffer[--i];
	}

	result[j] = '\0';
}
