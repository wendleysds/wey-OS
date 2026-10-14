#ifndef _CLIB_STANDART_IO_H
#define _CLIB_STANDART_IO_H

// print family
/*
printf()	        Escreve texto formatado em stdout.
fprintf()	        Escreve texto formatado em um FILE.
sprintf()	        Formata texto em um buffer, sem verificar o tamanho.
snprintf()	        Formata texto respeitando o tamanho do buffer.
vprintf()	        Versão de printf() que recebe va_list.
vfprintf()	        Versão de fprintf() que recebe va_list.
vsnprintf()	        Versão de snprintf() que recebe va_list.
puts()	            Escreve uma string seguida de \n.
fputs()	            Escreve uma string em um fluxo.
putchar()	        Escreve um caractere em stdout.
getchar()           Lê um caractere de stdin.
fputc()             Escreve um caractere em um fluxo.
fgetc()             Lê um caractere de um fluxo.
perror()            Escreve uma mensagem associada a errno.
*/

// file family
/*
fopen()     Abre um arquivo.
fclose()    Fecha um fluxo.
fread()     Lê blocos de dados.
fwrite()    Escreve blocos de dados.
fflush()    Descarrega os buffers de saída.
fseek()     Altera a posição no arquivo.
ftell()     Obtém a posição atual.
rewind()    Volta ao início do arquivo.
feof()      Verifica o indicador de fim de arquivo.
ferror()    Verifica o indicador de erro.
clearerr()  Limpa os indicadores de erro e EOF.
fileno()    Obtém o descritor associado ao fluxo.
fdopen()    Cria um fluxo a partir de um descritor existente.
*/

// input family
/*
scanf()	            Lê dados formatados de stdin.
fscanf()	        Lê dados formatados de um fluxo.
sscanf()	        Interpreta dados formatados a partir de uma string.
vscanf()	        Versão variádica baseada em va_list.
vfscanf()	        Versão de fscanf() com va_list.
vsscanf()           Versão de sscanf() com va_list.
*/

// stream family
/*
FILE                Estrutura que representa um fluxo de entrada/saída.
stdin               Fluxo de entrada padrão.
stdout              Fluxo de saída padrão.
stderr              Fluxo de erro padrão.
*/

// important macros
/*
EOF
BUFSIZ
FOPEN_MAX
SEEK_SET
SEEK_CUR
SEEK_END
stdin, stdout, stderr
*/

int putchar(int c);
int puts(const char* str);
int printf(const char *restrict fmt, ...);

#endif