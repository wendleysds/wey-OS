#ifndef _UTILS_H
#define _UTILS_H

/*
 * Utilities to use in all kind of siturations
 */

// Convert @value to @base (16, 8, 10, 2) and store in @result 
void itoa(int value, char* result, int base);

// Convert @str to upercase
void strupper(char* str);

#endif
