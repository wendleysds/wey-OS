#include <lib/utils.h>
#include <lib/string.h>
#include <stdint.h>

void itoa(int value, char* result, int base){
	const char* digits = "0123456789abcdef";
	char buffer[32];
	int i = 0, j = 0; 

	if (value < 0 && base == 10){
		result[j++] = '-';
		value = -value;
	}

	do {
		buffer[i++] = digits[value % base];
		value /= base;
	} while(value);

	while(i > 0){
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

