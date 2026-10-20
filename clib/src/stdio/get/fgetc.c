#include <unistd.h>
#include <errno.h>

#include "../internal.h"

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
