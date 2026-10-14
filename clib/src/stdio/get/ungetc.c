#include <string.h>

#include "../internal.h"

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
