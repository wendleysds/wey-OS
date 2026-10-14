#ifndef _CLIB_STRING_H
#define _CLIB_STRING_H

#include <stddef.h>

// memory manipulation
/**
 * @brief Copies n bytes from memory area src to memory area dest.
 * 
 * @param dest Pointer to the destination memory area.
 * @param src Pointer to the source memory area.
 * @param n Number of bytes to copy.
 * @return Pointer to the destination memory area.
 */
void* memcpy(void* dest, const void* src, size_t n);
/**
 * @brief Sets the first n bytes of memory area dest to the value c.
 * 
 * @param dest Pointer to the destination memory area.
 * @param c The value to set.
 * @param n Number of bytes to set.
 * @return Pointer to the destination memory area.
 */
void* memset(void* dest, int c, size_t n);
/**
 * @brief Compares n bytes of memory area a with memory area b.
 * 
 * @param a Pointer to the first memory area.
 * @param b Pointer to the second memory area.
 * @param n Number of bytes to compare.
 * @return 0 if the memory areas are equal, a non-zero value otherwise.
 */
int memcmp(const void* a, const void* b, size_t n);
/**
 * @brief Copies n bytes from memory area src to memory area dest, handling overlapping memory areas.
 * 
 * @param dest Pointer to the destination memory area.
 * @param src Pointer to the source memory area.
 * @param n Number of bytes to copy.
 * @return Pointer to the destination memory area.
 */
void* memmove(void* dest, const void* src, size_t n);

// string manipulation
/**
 * @brief Computes the length of the string s.
 * 
 * @param s The string to measure.
 * @return The length of the string.
 */
size_t strlen(const char* s);
/**
 * @brief Computes the length of the string s, up to a maximum of max.
 * 
 * @param s The string to measure.
 * @param max The maximum length to measure.
 * @return The length of the string.
 */
size_t strnlen(const char* s, size_t max);
/**
 * @brief Copies the string src to the memory area dest.
 * 
 * @param dest Pointer to the destination memory area.
 * @param src Pointer to the source string.
 * @return Pointer to the destination memory area.
 */
char* strcpy(char* dest, const char* src);
/**
 * @brief Copies at most n characters from the string src to the memory area dest.
 * 
 * @param dest Pointer to the destination memory area.
 * @param src Pointer to the source string.
 * @param n Number of characters to copy.
 * @return Pointer to the destination memory area.
 */
char* strncpy(char* dest, const char* src, size_t n);
/**
 * @brief Concatenates the string src to the string dest.
 * 
 * @param dest Pointer to the destination string.
 * @param src Pointer to the source string.
 * @return Pointer to the destination string.
 */
char* strcat(char* dest, const char* src);
/**
 * @brief Concatenates at most n characters from the string src to the string dest.
 * 
 * @param dest Pointer to the destination string.
 * @param src Pointer to the source string.
 * @param n Number of characters to concatenate.
 * @return Pointer to the destination string.
 */
char* strncat(char* dest, const char* src, size_t n);
/**
 * @brief Compares the string a with the string b.
 * 
 * @param a Pointer to the first string.
 * @param b Pointer to the second string.
 * @return 0 if the strings are equal, a non-zero value otherwise.
 */
int strcmp(const char* a, const char* b);
/**
 * @brief Compares the first n characters of the string a with the string b.
 * 
 * @param a Pointer to the first string.
 * @param b Pointer to the second string.
 * @param n Number of characters to compare.
 * @return 0 if the strings are equal, a non-zero value otherwise.
 */
int strncmp(const char* a, const char* b, size_t n);
/**
 * @brief Searches for the first occurrence of the character c in the memory area ptr.
 * 
 * @param ptr Pointer to the memory area to search.
 * @param c The character to search for.
 * @param n Number of bytes to search.
 * @return Pointer to the first occurrence of the character c, or NULL if not found.
 */
char* memchr(const void* ptr, int c, size_t n);
/**
 * @brief Searches for the first occurrence of the character c in the string s.
 * 
 * @param s The string to search.
 * @param c The character to search for.
 * @return Pointer to the first occurrence of the character c, or NULL if not found.
 */
char* strchr(const char* s, int c);
/**
 * @brief Searches for the last occurrence of the character c in the string s.
 * 
 * @param s The string to search.
 * @param c The character to search for.
 * @return Pointer to the last occurrence of the character c, or NULL if not found.
 */
char* strrchr(const char* s, int c);
/**
 * @brief Searches for the first occurrence of the substring needle in the string haystack.
 * 
 * @param haystack The string to search.
 * @param needle The substring to search for.
 * @return Pointer to the first occurrence of the substring needle, or NULL if not found.
 */
char* strstr(const char* haystack, const char* needle);
/**
 * @brief Breaks the string s into a sequence of tokens.
 * 
 * @param s The string to tokenize.
 * @param delimiters The characters to use as delimiters.
 * @return Pointer to the first token, or NULL if no tokens are found.
 */
char* strtok(char* s, const char* delimiters);
/**
 * @brief Transforms the string src into a string that can be used for comparison.
 * 
 * @param dest Pointer to the destination string.
 * @param src Pointer to the source string.
 * @param n Number of characters to transform.
 * @return The number of characters transformed.
 */
size_t strxfrm(char* dest, const char* src, size_t n);

#endif
