#ifndef _CLIB_STANDART_LIBRARY_H
#define _CLIB_STANDART_LIBRARY_H

#include <stddef.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#define RAND_MAX (2147483647)

#ifndef NULL
#define NULL ((void *)0)
#endif

// memory management
/**
 * @brief Allocate memory.
 * 
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL if an error occurred.
 */
void *malloc(size_t size);

/**
 * @brief Allocate memory and initialize it with zeros.
 * 
 * @param nmemb The number of elements to allocate.
 * @param size The size of each element.
 * @return A pointer to the allocated memory, or NULL if an error occurred.
 */
void *calloc(size_t nmemb, size_t size);

/**
 * @brief Resize a memory block.
 * 
 * @param ptr A pointer to the memory block to resize.
 * @param nsize The new size of the memory block.
 * @return A pointer to the resized memory block, or NULL if an error occurred.
 */
void* realloc(void* ptr, size_t nsize);

/**
 * @brief Free a memory block.
 * 
 * @param ptr A pointer to the memory block to free.
 */
void free(void *ptr);

// system
/**
 * @brief Terminates the program abnormally.
 */
void abort(void);

/**
 * @brief Register a function to be called when the program terminates normally.
 * 
 * @param func The function to register.
 * @return 0 if successful, non-zero otherwise.
 */
int atexit(void (*func)(void));

/** 
 * @brief Get the value of an environment variable.
 * 
 * @param name The name of the environment variable to get.
 * @return The value of the environment variable, or NULL if not found.
 */
char *getenv(const char *name);

/**
 * @brief Execute a command by means of a command processor.
 * 
 * @param command The command to execute.
 * @return The return status of the command, or -1 if an error occurred.
 */
int system(const char *command);

/**
 * @brief Terminates the program normally.
 * 
 * @param status The exit status.
 */
void exit(int status) __attribute__((noreturn));

/**
 * @brief Terminates the program immediately.
 * 
 * @param status The exit status.
 */
void _Exit(int status) __attribute__((noreturn));

/**
 * @brief Terminates the program abnormally.
 * 
 */
void abort(void) __attribute__((noreturn));

// conversion
/**
 * @brief Convert string to int.
 * @param str The string to convert.
 * @return The int value.
 */
int atoi(const char *str);

/**
 * @brief Convert string to long.
 * @param str The string to convert.
 * @return The long value.
 */
long atol(const char *str);

/**
 * @brief Convert string to long long.
 * @param str The string to convert.
 * @return The long long value.
 */
long long atoll(const char *str);

/**
 * @brief Convert string to unsigned long.
 * @param str The string to convert.
 * @return The unsigned long value.
 */
unsigned long strtoul(const char *str, char **endptr, int base);

/**
 * @brief Convert string to long.
 * @param str The string to convert.
 * @return The long value.
 */
long strtol(const char *str, char **endptr, int base);

/**
 * @brief Convert string to long long.
 * @param str The string to convert.
 * @return The long long value.
 */
long long strtoll(const char *str, char **endptr, int base);

/**
 * @brief Convert string to unsigned long long.
 * @param str The string to convert.
 * @return The unsigned long long value.
 */
unsigned long long strtoull(const char *str, char **endptr, int base);

/**
 * @brief Convert unsigned int to string.
 * @param val The unsigned int to convert.
 * @param str The string to convert to.
 * @param size The size of the string buffer.
 * @param base The base to convert to.
 * @return The string.
 */
char *utoa(unsigned int val, char *str, size_t size, int base);

/**
 * @brief Convert int to string.
 * @param val The int to convert.
 * @param str The string to convert to.
 * @param size The size of the string buffer.
 * @param base The base to convert to.
 * @return The string.
 */
char *itoa(int val, char *str, size_t size, int base);

/**
 * @brief Convert long long to string.
 * @param val The long long to convert.
 * @param str The string to convert to.
 * @param size The size of the string buffer.
 * @param base The base to convert to.
 * @return The string.
 */
char *lltoa(long long val, char *str, size_t size, int base);

/**
 * @brief Convert unsigned long long to string.
 * @param val The unsigned long long to convert.
 * @param str The string to convert to.
 * @param size The size of the string buffer.
 * @param base The base to convert to.
 * @return The string.
 */
char *ulltoa(unsigned long long val, char *str, size_t size, int base);

float strtof(const char *str, char **endptr);

// utils
/*
qsort()     Ordena um array usando um comparador.
bsearch()   Busca um elemento em um array ordenado.
rand()      Gera números pseudoaleatórios.
srand()     Define a semente do gerador de rand().
*/

// math
typedef struct {
    int quot;
    int rem;
} div_t;

typedef struct {
    long quot;
    long rem;
} ldiv_t;

typedef struct {
    long long quot;
    long long rem;
} lldiv_t;

/**
 * @brief Calculate the absolute value of an int.
 * @param x The int to calculate the absolute value of.
 * @return The absolute value of x.
 */
int abs(int x);

/**
 * @brief Calculate the absolute value of a long.
 * @param x The long to calculate the absolute value of.
 * @return The absolute value of x.
 */
long labs(long x);

/**
 * @brief Calculate the absolute value of a long long.
 * @param x The long long to calculate the absolute value of.
 * @return The absolute value of x.
 */
long long llabs(long long x);

/**
 * @brief Calculate the quotient and remainder of an int division.
 * @param numer The numerator.
 * @param denom The denominator.
 * @return The quotient and remainder.
 */
div_t div(int numer, int denom);

/**
 * @brief Calculate the quotient and remainder of a long division.
 * @param numer The numerator.
 * @param denom The denominator.
 * @return The quotient and remainder.
 */
ldiv_t ldiv(long numer, long denom);

/**
 * @brief Calculate the quotient and remainder of a long long division.
 * @param numer The numerator.
 * @param denom The denominator.
 * @return The quotient and remainder.
 */
lldiv_t lldiv(long long numer, long long denom);

#endif /* _CLIB_STANDART_LIBRARY_H */
