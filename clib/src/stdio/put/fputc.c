#include <unistd.h>

#include "../internal.h"

int fputc(int c, FILE* stream){
    if (!stream) return EOF;

    if (stream->pos >= stream->capacity) {
        if (fflush(stream) < 0) return EOF;
    }

    if(stream->flags & _FILE_UNBUF){
        long res = write(stream->fd, &c, 1);
        if (res <= 0) {
            if (res < 0) stream->flags |= _FILE_ERR;
            else stream->flags |= _FILE_EOF;
            return EOF;
        }
        return (unsigned char)c;
    }

    stream->buffer[stream->pos++] = (unsigned char)c;
    return (unsigned char)c;
}
