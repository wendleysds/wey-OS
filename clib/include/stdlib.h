#ifndef _CLIB_STANDART_LIBRARY_H
#define _CLIB_STANDART_LIBRARY_H

#include <stddef.h>

// memory management
/*
malloc()    Aloca memória dinamicamente.
calloc()    Aloca e inicializa com zeros.
realloc()   Redimensiona um bloco.
free()      Libera um bloco.
*/

// system
/*
abort()     Encerra o programa anormalmente.
exit()      Encerra o programa normalmente.
_Exit()     Encerra sem executar handlers nem descarregar buffers da libc.
atexit()    Registra funções para execução no encerramento normal.
getenv()    Consulta uma variável de ambiente.
system()    Executa um comando por meio de um processador de comandos.
*/

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

// utils
/*
qsort()     Ordena um array usando um comparador.
bsearch()   Busca um elemento em um array ordenado.
rand()      Gera números pseudoaleatórios.
srand()     Define a semente do gerador de rand().
*/

// math
/*
abs()       Valor absoluto de um int.
labs()      Valor absoluto de um long.
llabs()     Valor absoluto de um long long.
div()       Retorna quociente e resto de uma divisão inteira.
ldiv()      Variante para long.
lldiv()     Variante para long long.
*/

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

div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);

// important macros
/*
EXIT_SUCCESS
EXIT_FAILURE
RAND_MAX
NULL
MB_CUR_MAX
*/

#endif /* _CLIB_STANDART_LIBRARY_H */