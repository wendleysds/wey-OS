#include <unistd.h>

#include "../internal.h"

int fputc(int c, FILE* stream) {
    if (!stream) return EOF;

    if (stream->flags & _FILE_UNBUF) {
        long res = write(stream->fd, &c, 1);
        if (res <= 0) {
            if (res < 0) stream->flags |= _FILE_ERR;
            else stream->flags |= _FILE_EOF;
            return EOF;
        }
        return (unsigned char)c;
    }

    if (stream->capacity == 0 || stream->pos >= stream->capacity) {
        if (fflush(stream) < 0) return EOF;
    }

    if (stream->capacity == 0 || !stream->buffer) return EOF;

    stream->buffer[stream->pos++] = (unsigned char)c;
    stream->flags |= _FILE_DIRTY;

    if ((stream->flags & _FILE_LINEBUF) && (char)c == '\n') {
        if (fflush(stream) < 0) return EOF;
    }

    return (unsigned char)c;
}
