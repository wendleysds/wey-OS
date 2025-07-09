#ifndef _UTILS_H
#define _UTILS_H

/*
 * Utilities to use in all kind of siturations
 */

// Convert @value to @base (16, 8, 10, 2) and store in @result 
void itoa(int value, char* result, int base);

// Convert @str to upercase
void strupper(char* str);

// Format a @filename to fit in FAT32 8.3 format and store in @out[12]
void format_fat_name(const char* filename, char* out);

#endif
