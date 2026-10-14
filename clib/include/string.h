#ifndef _CLIB_STRING_H
#define _CLIB_STRING_H

// string manipulation
/*
strlen()    Calcula o tamanho.
strcpy()    Copia string.
strncpy()   Copia parte da string.
strcat()    Concatena.
strncat()   Concatena parte da string.
strcmp()    Compara strings.
strncmp()   Compara parte da string.
memchr()    Busca caractere em memória.
memcmp()    Compara blocos de memória.
memcpy()    Copia blocos de memória.
memmove()   Copia blocos de memória (safe).
memset()    Preenche memória com valor constante.
strchr()    Busca caractere na string.
strrchr()   Busca último caractere na string.
strstr()    Busca substring.
strtok()    Tokeniza string.
strxfrm()  Transforma string para comparação.
*/

int strlen(const char *s);

#endif
