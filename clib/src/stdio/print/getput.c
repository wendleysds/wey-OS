#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "../internal.h"

int puts(const char* str){
    return fprintf(stdin, "%s\n", str);
}

int fputs(const char* str, FILE* stream){
    if (!stream) return EOF;
    return fprintf(stream, "%s", str);
}

int putchar(int c){
    return putc(c, stdout);
}

int getchar(void){
    return fgetc(stdin);
}

int putc(int c, FILE* stream){
    if (stream == NULL) return EOF;
    return fputc(c, stream);
}

int fputc(int c, FILE* stream){
    if (!stream) return EOF;

    if (stream->pos >= stream->capacity) {
        if (fflush(stream) < 0) return EOF;
    }

    stream->buffer[stream->pos++] = (unsigned char)c;
    return (unsigned char)c;
}

int fgetc(FILE *stream) {
    if (!stream) return EOF;

    if (stream->ungetc_buf != -1) {
        int c = stream->ungetc_buf;
        stream->ungetc_buf = -1;
        return c;
    }
    
    if (stream->pos >= stream->size) {
        if (stream->flags & _FILE_UNBUF) {
            unsigned char c;
            long res = read(stream->fd, &c, 1);
            if (res <= 0) {
                if (res < 0) stream->flags |= _FILE_ERR;
                else stream->flags |= _FILE_EOF;
                return EOF;
            }
            return c;
        }
        
        stream->pos = 0;
        long res;
        do {
            res = read(stream->fd, stream->buffer, stream->capacity);
        } while (res < 0 && errno == EINTR);
        
        if (res <= 0) {
            stream->size = 0;
            if (res < 0) stream->flags |= _FILE_ERR;
            else stream->flags |= _FILE_EOF;
            return EOF;
        }
        stream->size = (size_t)res;
    }
    
    return (unsigned char)stream->buffer[stream->pos++];
}

int ungetc(int c, FILE *stream) {
    if (!stream || c == EOF)
        return EOF;

    if (stream->flags & _FILE_WRITE) 
        return EOF;

    if (stream->flags & _FILE_UNBUF){
        if (stream->ungetc_buf != -1) return EOF;
        stream->ungetc_buf = c;
        return c;
    }

    if (stream->pos > 0) {
        stream->pos--;
    } 
    else {
        if (stream->size >= stream->capacity) {
            if (stream->ungetc_buf != -1) return EOF;
            stream->ungetc_buf = c;
            return c;
        }

        if (stream->size > 0) {
            memmove(stream->buffer + 1, stream->buffer, stream->size);
        }
        
        stream->pos = 0;
        stream->size++;
    }

    stream->buffer[stream->pos] = (unsigned char)c;
    stream->flags &= ~_FILE_EOF;
    return c;
}
