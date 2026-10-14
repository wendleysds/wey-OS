#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "../internal.h"

FILE *freopen(const char *restrict filename, const char *restrict mode, FILE *restrict stream) {
    if (!stream || !mode) {
        return NULL;
    }

    int open_flags = __mode_to_open_flags(mode);
    if (open_flags == -1) {
        return NULL;
    }

    int file_flags = __mode_to_file_flags(mode);
    if (file_flags == -1) {
        return NULL;
    }

    /* Flush pending writes */
    if (stream->flags & _FILE_DIRTY) {
        fflush(stream);
    }

    /* Release any buffer allocated by libc */
    if ((stream->flags & _FILE_OWNBUF) && stream->buffer) {
        free(stream->buffer);
        stream->buffer = NULL;
        stream->flags &= ~_FILE_OWNBUF;
    }

    /* Close the existing file descriptor */
    if (stream->fd >= 0) {
        close(stream->fd);
        stream->fd = -1;
    }

    if (!filename) {
        return NULL;
    }

    int new_fd = open(filename, open_flags, 0666);
    if (new_fd < 0) {
        return NULL;
    }

    /* Reset stream state */
    stream->fd = new_fd;
    stream->flags = file_flags | _FILE_UNBUF;
    stream->buffer = NULL;
    stream->capacity = 0;
    stream->size = 0;
    stream->pos = 0;
    stream->ungetc_buf = -1;

    /* Ensure stream is tracked for flush/close on exit */
    __file_register(stream);

    return stream;
}
