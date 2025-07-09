#include <lib/utils.h>
#include <stdint.h>

void itoa(int value, char* result, uint8_t base){
	const char* digits = "0123456789ABCDEF";
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

void strupper(char* str){
	for(int i = 0; str[i]; i++){
		if(str[i] >= 'a' && str[i] <= 'z'){
			str[i] -= 32;
		}
	}
}

void format_fat_name(const char* filename, char* out){
	memset(out, ' ', 11);

	if(!filename || strlen(filename) == 0){
		out[0] = '\0';
		return;
	}

	if(strlen(filename) > 11){
		// If the filename is longer than 11 characters, truncate it
		strncpy(out, filename, 8);
		
		// Copy extension (last 3 chars after dot, if present)
		const char* dot = strrchr(filename, '.');
		if (dot && *(dot + 1)) {
			const char* ext = dot + 1;
			uint32_t extLen = strlen(ext);
			if (extLen > 3) ext += extLen - 3;
			strcpy(out + 8, ext);
		} else {
			memset(out + 8, ' ', 3);
		}
		
		out[6] = '~'; // Indicate that the name is truncated
		out[7] = '1'; // Add a number to differentiate
		out[11] = '\0';

		strupper(out);

		return;
	}

	const char* dot = strrchr(filename, '.');
	uint8_t nameLenght = 0;
	uint8_t extLenght = 0;

	for(const char* p = filename; *p && p != dot && nameLenght < 8; p++){
		out[nameLenght++] = (uint8_t)*p;
	}

	if(dot && *(dot + 1)){
		const char* ext = dot + 1;
		while(*ext && extLenght < 3){
			out[8 + extLenght++] = (uint8_t)*ext;
			ext++;
		}
	}

	out[11] = '\0';

	strupper(out);
}