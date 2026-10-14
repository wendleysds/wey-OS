#include <stdlib.h>
#include <string.h>

#include "../internal.h"

int setvbuf(FILE *restrict stream, char *restrict buf, int mode, size_t size) {
    if (!stream) {
        return -1;
    }

    if (mode != _IONBF && mode != _IOLBF && mode != _IOFBF) {
        return -1;
    }

    /* Flush pending data before altering the buffer */
    fflush(stream);

    /* Release any previously allocated buffer owned by libc */
    if ((stream->flags & _FILE_OWNBUF) && stream->buffer) {
        free(stream->buffer);
        stream->buffer = NULL;
        stream->flags &= ~_FILE_OWNBUF;
    }

    if (mode == _IONBF) {
        stream->buffer = NULL;
        stream->capacity = 0;
        stream->size = 0;
        stream->pos = 0;
        stream->flags |= _FILE_UNBUF;
        stream->flags &= ~_FILE_LINEBUF;
        return 0;
    }

    if (buf == NULL) {
        if (size == 0) {
            size = BUFSIZ;
        }
        char *new_buf = malloc(size);
        if (!new_buf) {
            return -1;
        }
        stream->buffer = new_buf;
        stream->capacity = size;
        stream->flags |= _FILE_OWNBUF;
    } else {
        if (size == 0) {
            return -1;
        }
        stream->buffer = buf;
        stream->capacity = size;
        stream->flags &= ~_FILE_OWNBUF;
    }

    stream->size = 0;
    stream->pos = 0;
    stream->flags &= ~_FILE_UNBUF;

    if (mode == _IOLBF) {
        stream->flags |= _FILE_LINEBUF;
    } else {
        stream->flags &= ~_FILE_LINEBUF;
    }

    return 0;
}

void setbuf(FILE *restrict stream, char *restrict buf) {
    (void)setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}
