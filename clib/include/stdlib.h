#ifndef _CLIB_STANDART_LIBRARY_H
#define _CLIB_STANDART_LIBRARY_H

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
/*
atoi()      Converte string em inteiro, sem tratamento robusto de erros.
atol()      Converte string em long.
strtol()    Converte string em inteiro com controle de erros.
strtoul()   Converte string em inteiro sem sinal.
strtoll()   Converte string em long long.
strtoull()  Converte string em unsigned long long.
strtof()    Converte string em float.
strtod()    Converte string em double.
strtold()   Converte string em long double.
*/

// sorting
/*
qsort()     Ordena um array usando um comparador.
bsearch()   Busca um elemento em um array ordenado.
rand()      Gera números pseudoaleatórios.
srand()     Define a semente do gerador de rand().
abs()       Valor absoluto de um int.
labs()      Valor absoluto de um long.
llabs()     Valor absoluto de um long long.
div()       Retorna quociente e resto de uma divisão inteira.
ldiv()      Variante para long.
lldiv()     Variante para long long.
*/

// important macros
/*
EXIT_SUCCESS
EXIT_FAILURE
RAND_MAX
NULL
MB_CUR_MAX
*/


void itoa(int value, char* result, int base);
void utoa(unsigned int value, char* result, int base);

#endif