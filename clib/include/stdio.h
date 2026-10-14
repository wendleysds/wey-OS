#ifndef _CLIB_STDIO_H
#define _CLIB_STDIO_H

#include <stddef.h>
#include <stdarg.h>

#define SEEK_SET    (0)
#define SEEK_CUR    (1)
#define SEEK_END    (2)
#define EOF (-1)

#define _IONBF (0)
#define _IOLBF (1)
#define _IOFBF (2)

#define FOPEN_MAX (20)
#define BUFSIZ (1024)
#define L_tmpnam (20)
#define TMP_MAX (10000)
#define P_tmpdir "/tmp"

typedef struct file FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define stdin stdin
#define stdout stdout
#define stderr stderr

#define getc(stream) fgetc(stream)

// print family
/**
 * @brief Prints formatted text to stdout.
 * 
 * @param fmt The format string.
 * @param ... The arguments to print.
 * @return The number of characters printed, or -1 if an error occurred.
 */
int printf(const char *restrict fmt, ...);

/**
 * @brief Prints formatted text to a file.
 * 
 * @param stream A pointer to the FILE object to print to.
 * @param fmt The format string.
 * @param ... The arguments to print.
 * @return The number of characters printed, or -1 if an error occurred.
 */
int fprintf(FILE* restrict stream, const char* restrict fmt, ...);

/**
 * @brief Formats text into a buffer without checking the size.
 * 
 * @param str A pointer to the buffer to write the formatted text to.
 * @param fmt The format string.
 * @param ... The arguments to format.
 * @return The number of characters formatted, or -1 if an error occurred.
 */
int sprintf(char *restrict str, const char *restrict fmt, ...);

/**
 * @brief Formats text into a buffer while checking the size.
 * 
 * @param buf A pointer to the buffer to write the formatted text to.
 * @param size The size of the buffer.
 * @param fmt The format string.
 * @param ... The arguments to format.
 * @return The number of characters formatted, or -1 if an error occurred.
 */
int snprintf(char* restrict buf, size_t size, const char* restrict fmt, ...);

/**
 * @brief Prints formatted text to stdout using va_list.
 * 
 * @param fmt The format string.
 * @param args The arguments to print.
 * @return The number of characters printed, or -1 if an error occurred.
 */
int vprintf(const char* fmt, va_list args);

/**
 * @brief Prints formatted text to a file using va_list.
 * 
 * @param stream A pointer to the FILE object to print to.
 * @param fmt The format string.
 * @param args The arguments to print.
 * @return The number of characters printed, or -1 if an error occurred.
 */
int vfprintf(FILE* restrict stream, const char* restrict fmt, va_list args);

/**
 * @brief Formats text into a buffer using va_list while checking the size.
 * 
 * @param buf A pointer to the buffer to write the formatted text to.
 * @param size The size of the buffer.
 * @param fmt The format string.
 * @param args The arguments to format.
 * @return The number of characters formatted, or -1 if an error occurred.
 */
int vsnprintf(char *restrict buf, size_t size, const char *fmt, va_list args);

/**
 * @brief Formats text into a buffer using va_list without checking the size.
 * 
 * @param str A pointer to the buffer to write the formatted text to.
 * @param fmt The format string.
 * @param args The arguments to format.
 * @return The number of characters formatted, or -1 if an error occurred.
 */
int vsprintf(char *restrict str, const char *restrict fmt, va_list args);

/**
 * @brief Prints an error message to stderr.
 * 
 * @param s The error message to print.
 * @return The number of characters printed, or -1 if an error occurred.
 */
int perror(const char *s);

/**
 * @brief Writes a string to stdout.
 * 
 * @param str The string to write.
 * @return The number of characters written, or -1 if an error occurred.
 */
int puts(const char* str);

/**
 * @brief Writes a string to a file.
 * 
 * @param str The string to write.
 * @param stream A pointer to the FILE object to write to.
 * @return The number of characters written, or -1 if an error occurred.
 */
int fputs(const char* str, FILE* stream);

/**
 * @brief Writes a character to stdout.
 * 
 * @param c The character to write.
 * @return The character written, or EOF if an error occurred.
 */
int putchar(int c);

/**
 * @brief Reads a character from stdin.
 * 
 * @return The character read, or EOF if an error occurred.
 */
int getchar(void);

/**
 * @brief Reads a line from a file stream.
 * 
 * @param str Buffer to store the read string.
 * @param size Maximum number of characters to read (including terminating null byte).
 * @param stream A pointer to the FILE object to read from.
 * @return str on success, or NULL on error or EOF with no characters read.
 */
char *fgets(char *restrict str, int size, FILE *restrict stream);

/**
 * @brief Writes a character to a file.
 * 
 * @param c The character to write.
 * @param stream A pointer to the FILE object to write to.
 * @return The character written, or EOF if an error occurred.
 */
int putc(int c, FILE* stream);

/**
 * @brief Reads a character from a file.
 * 
 * @param stream A pointer to the FILE object to read from.
 * @return The character read, or EOF if an error occurred.
 */
int fgetc(FILE* stream);

/**
 * @brief Writes a character to a file.
 * 
 * @param c The character to write.
 * @param stream A pointer to the FILE object to write to.
 * @return The character written, or EOF if an error occurred.
 */
int putc(int c, FILE* stream);

/**
 * @brief Writes a character to a file.
 * 
 * @param c The character to write.
 * @param stream A pointer to the FILE object to write to.
 * @return The character written, or EOF if an error occurred.
 */
int fputc(int c, FILE* stream);

// file family
/**
 * @brief Opens a file.
 * 
 * @param filename The name of the file to open.
 * @param mode The mode to open the file with.
 * @return A pointer to the FILE object, or NULL if an error occurred.
 */
FILE* fopen(const char *filename, const char *mode);

/**
 * @brief Closes a file.
 * 
 * @param stream A pointer to the FILE object to close.
 * @return 0 if successful, or EOF if an error occurred.
 */
int fclose(FILE *stream);

/**
 * @brief Reads blocks of data from a file.
 * 
 * @param ptr A pointer to the buffer to read data into.
 * @param size The size of each block to read.
 * @param nmemb The number of blocks to read.
 * @param stream A pointer to the FILE object to read data from.
 * @return The number of blocks read, or 0 if an error occurred.
 */
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

/**
 * @brief Writes blocks of data to a file.
 * 
 * @param ptr A pointer to the buffer to write data from.
 * @param size The size of each block to write.
 * @param nmemb The number of blocks to write.
 * @param stream A pointer to the FILE object to write data to.
 * @return The number of blocks written, or 0 if an error occurred.
 */
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/**
 * @brief Flushes the output buffer.
 * 
 * @param stream A pointer to the FILE object to flush.
 * @return 0 if successful, or EOF if an error occurred.
 */
int fflush(FILE *stream);

/**
 * @brief Changes the current position in a file.
 * 
 * @param stream A pointer to the FILE object to change the position in.
 * @param offset The offset to change the position by.
 * @param whence The whence to change the position by.
 * @return 0 if successful, or -1 if an error occurred.
 */
int fseek(FILE *stream, long offset, int whence);

/**
 * @brief Gets the current position in a file.
 * 
 * @param stream A pointer to the FILE object to get the position of.
 * @return The current position in the file, or -1 if an error occurred.
 */
long ftell(FILE *stream);

/**
 * @brief Rewinds the file to the beginning of the file.
 * 
 * @param stream A pointer to the FILE object to rewind.
 */
void rewind(FILE *stream);

/**
 * @brief Checks if the end of the file has been reached.
 * 
 * @param stream A pointer to the FILE object to check.
 * @return 1 if the end of the file has been reached, or 0 otherwise.
 */
int feof(FILE *stream);

/**
 * @brief Checks if an error has occurred.
 * 
 * @param stream A pointer to the FILE object to check.
 * @return 1 if an error has occurred, or 0 otherwise.
 */
int ferror(FILE *stream);

/**
 * @brief Clears the error and EOF indicators.
 * 
 * @param stream A pointer to the FILE object to clear.
 */
void clearerr(FILE *stream);

/**
 * @brief Gets the file descriptor associated with the stream.
 * 
 * @param stream A pointer to the FILE object to get the file descriptor of.
 * @return The file descriptor, or -1 if an error occurred.
 */
int fileno(FILE *stream);

/**
 * @brief Creates a FILE object from a file descriptor.
 * 
 * @param fd The file descriptor to create the FILE object from.
 * @param mode The mode to create the FILE object with.
 * @return A pointer to the FILE object, or NULL if an error occurred.
 */
FILE* fdopen(int fd, const char* mode);

/**
 * @brief Pushes a character back into the input stream.
 * 
 * @param c The character to push back.
 * @param stream A pointer to the FILE object.
 * @return The character pushed back, or EOF if an error occurred.
 */
int ungetc(int c, FILE *stream);

// input family
/**
 * @brief Reads formatted input from standard input.
 * 
 * @param format The format string.
 * @param args The arguments to format.
 * @return The number of items successfully read, or EOF if an error occurred.
 */
int vscanf(const char *restrict format, va_list args);

/**
 * @brief Reads formatted input from a string.
 * 
 * @param s The string to read from.
 * @param format The format string.
 * @param args The arguments to format.
 * @return The number of items successfully read, or EOF if an error occurred.
 */
int vsscanf(const char *restrict s, const char *restrict format, va_list args);

/**
 * @brief Reads formatted input from a file.
 * 
 * @param stream A pointer to the FILE object to read from.
 * @param format The format string.
 * @param args The arguments to format.
 * @return The number of items successfully read, or EOF if an error occurred.
 */
int vfscanf(FILE *restrict stream, const char *restrict format, va_list args);

/**
 * @brief Reads formatted input from a string.
 * 
 * @param s The string to read from.
 * @param format The format string.
 * @param ... The arguments to format.
 * @return The number of items successfully read, or EOF if an error occurred.
 */
int sscanf(const char *restrict s, const char *restrict format, ...);

/**
 * @brief Reads formatted input from a file.
 * 
 * @param stream A pointer to the FILE object to read from.
 * @param format The format string.
 * @param ... The arguments to format.
 * @return The number of items successfully read, or EOF if an error occurred.
 */
int fscanf(FILE *restrict stream, const char *restrict fmt, ...);

/**
 * @brief Reads formatted input from standard input.
 * 
 * @param format The format string.
 * @param ... The arguments to format.
 * @return The number of items successfully read, or EOF if an error occurred.
 */
int scanf(const char *restrict fmt, ...);

/**
 * @brief Create and open a temporary file in binary read/write mode.
 * 
 * @return A pointer to the FILE object, or NULL if an error occurred.
 */
FILE *tmpfile(void);

/**
 * @brief Generate a temporary filename.
 * 
 * @param buf The buffer to store the filename.
 * @return A pointer to the filename, or NULL if an error occurred.
 */
char *tmpnam(char *buf);

/**
 * @brief Set the buffering mode for a stream.
 * 
 * @param stream The stream to set the buffering mode for.
 * @param buf The buffer to use for buffering.
 * @param mode The buffering mode.
 * @param size The size of the buffer.
 * @return 0 if successful, or EOF if an error occurred.
 */
int setvbuf(FILE *restrict stream, char *restrict buf, int mode, size_t size);

/**
 * @brief Set the buffering mode for a stream.
 * 
 * @param stream The stream to set the buffering mode for.
 * @param buf The buffer to use for buffering.
 */
void setbuf(FILE *restrict stream, char *restrict buf);

/**
 * @brief Replaces a file with a specified file.
 * 
 * @param restrict 
 * @param restrict 
 * @param restrict 
 * @return FILE* 
 */
FILE* freopen(const char* restrict, const char* restrict, FILE* restrict);

/**
 * @brief Removes a file.
 * 
 * @param pathname The name of the file to remove.
 * @return 0 if successful, or EOF if an error occurred.
 */
int remove(const char *pathname);

/**
 * @brief Renames a file.
 * 
 * @param old The old name of the file.
 * @param new The new name of the file.
 * @return 0 if successful, or EOF if an error occurred.
 */
int rename(const char *old, const char *new);

#endif /* _CLIB_STDIO_H */
