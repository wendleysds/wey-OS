#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#include "../internal.h"

static int __flush_write_buffer(FILE *stream) {
    if (stream->flags & _FILE_UNBUF)
        return 0;

    size_t written = 0;
    size_t pending = stream->pos;

    while (written < pending) {
        long res = write(
            stream->fd,
            stream->buffer + written,
            pending - written
        );

        if (res < 0) {
            if (errno == EINTR)
                continue;

            stream->flags |= _FILE_ERR;

            if (written > 0) {
                memmove(
                    stream->buffer,
                    stream->buffer + written,
                    pending - written
                );
                stream->pos = pending - written;
            }

            return -1;
        }

        if (res == 0)
            break;

        written += (size_t)res;
    }

    if (written < pending) {
        memmove(
            stream->buffer,
            stream->buffer + written,
            pending - written
        );
        stream->pos = pending - written;
        stream->flags |= _FILE_DIRTY;
        return -1;
    }

    stream->pos = 0;
    stream->size = 0;
    stream->flags &= ~_FILE_DIRTY;

    return 0;
}

static int __discard_read_buffer(FILE *stream)
{
    if (stream->flags & _FILE_UNBUF)
        return 0;

    if (stream->size > stream->pos) {
        size_t unread = stream->size - stream->pos;
        if (unread > (size_t)LONG_MAX) return -1;

        if (lseek(stream->fd, -(long)unread, SEEK_CUR) < 0) {
            stream->flags |= _FILE_ERR;
            return -1;
        }
    }

    stream->pos = 0;
    stream->size = 0;

    return 0;
}

int fflush(FILE *stream)
{
    if (!stream) return 0;

    stream->ungetc_buf = -1;

    if (!(stream->flags & _FILE_WRITE)) {
        if (stream->flags & _FILE_READ) {
            return __discard_read_buffer(stream);
        }
        return 0;
    }

    if (stream->flags & _FILE_UNBUF)
        return 0;

    if (!(stream->flags & _FILE_DIRTY))
        return 0;

    return __flush_write_buffer(stream);
}

size_t fread(
    void *ptr,
    size_t size,
    size_t nmemb,
    FILE *stream
){
    if (!stream || size == 0 || nmemb == 0)
        return 0;

    if (!ptr || !(stream->flags & _FILE_READ))
        return 0;

    if (nmemb > LONG_MAX / size)
        return 0;

    if (stream->flags & _FILE_DIRTY) {
        if (fflush(stream) != 0)
            return 0;
    }

    unsigned char *dst = ptr;
    size_t bytes_to_read = size * nmemb;
    size_t bytes_read = 0;

    if (stream->ungetc_buf != -1 && bytes_to_read > 0) {
        int c = stream->ungetc_buf;
        stream->ungetc_buf = -1;
        
        dst[0] = (unsigned char)c;
        bytes_read += 1;
    }

    if (stream->flags & _FILE_UNBUF) {
        while (bytes_read < bytes_to_read) {
            size_t remaining = bytes_to_read - bytes_read;
            long res = read(stream->fd, dst + bytes_read, remaining);

            if (res < 0) {
                if (errno == EINTR)
                    continue;
                stream->flags |= _FILE_ERR;
                break;
            }

            if (res == 0) {
                stream->flags |= _FILE_EOF;
                break;
            }

            bytes_read += (size_t)res;
        }
        return bytes_read / size;
    }

    while (bytes_read < bytes_to_read) {

        if (stream->pos < stream->size) {
            size_t available = stream->size - stream->pos;
            size_t remaining = bytes_to_read - bytes_read;
            size_t chunk = remaining < available
                         ? remaining
                         : available;

            memcpy(
                dst + bytes_read,
                stream->buffer + stream->pos,
                chunk
            );

            stream->pos += chunk;
            bytes_read += chunk;
            continue;
        }

        stream->pos = 0;
        stream->size = 0;

        size_t remaining = bytes_to_read - bytes_read;

        if (stream->capacity == 0 ||
            remaining >= stream->capacity) {
            long res = read(
                stream->fd,
                dst + bytes_read,
                remaining
            );

            if (res < 0) {
                if (errno == EINTR)
                    continue;

                stream->flags |= _FILE_ERR;
                break;
            }

            if (res == 0) {
                stream->flags |= _FILE_EOF;
                break;
            }

            bytes_read += (size_t)res;
            continue;
        }

        long res = read(
            stream->fd,
            stream->buffer,
            stream->capacity
        );

        if (res < 0) {
            if (errno == EINTR)
                continue;

            stream->flags |= _FILE_ERR;
            break;
        }

        if (res == 0) {
            stream->flags |= _FILE_EOF;
            break;
        }

        stream->size = (size_t)res;
    }

    return bytes_read / size;
}

size_t fwrite(
    const void *ptr,
    size_t size,
    size_t nmemb,
    FILE *stream
){
    if (!stream || size == 0 || nmemb == 0)
        return 0;

    if (!ptr || !(stream->flags & _FILE_WRITE))
        return 0;

    if (nmemb > LONG_MAX / size)
        return 0;

    if (!(stream->flags & _FILE_DIRTY) &&
        stream->size > stream->pos) {
        if (__discard_read_buffer(stream) != 0)
            return 0;
    }

    const unsigned char *src = ptr;
    size_t bytes_to_write = size * nmemb;
    size_t bytes_written = 0;

    stream->ungetc_buf = -1;

    if (stream->capacity == 0 || (stream->flags & _FILE_UNBUF)) {
        while (bytes_written < bytes_to_write) {
            long res = write(
                stream->fd,
                src + bytes_written,
                bytes_to_write - bytes_written
            );

            if (res < 0) {
                if (errno == EINTR)
                    continue;

                stream->flags |= _FILE_ERR;
                break;
            }

            if (res == 0) {
                stream->flags |= _FILE_ERR;
                break;
            }

            bytes_written += (size_t)res;
        }

        return bytes_written / size;
    }

    while (bytes_written < bytes_to_write) {
        if (stream->pos >= stream->capacity) {
            if (fflush(stream) != 0)
                break;
        }

        size_t space_left = stream->capacity - stream->pos;
        size_t remaining = bytes_to_write - bytes_written;
        size_t chunk = remaining < space_left
                     ? remaining
                     : space_left;

        memcpy(
            stream->buffer + stream->pos,
            src + bytes_written,
            chunk
        );

        stream->pos += chunk;
        bytes_written += chunk;
        stream->flags |= _FILE_DIRTY;
    }

    return bytes_written / size;
}

long ftell(FILE *stream) {
    if (!stream)
        return -1;

    long kernel_pos = lseek(stream->fd, 0, SEEK_CUR);
    if (kernel_pos < 0)
        return -1;

    if (stream->flags & _FILE_UNBUF) {
        if (stream->ungetc_buf != -1) {
            kernel_pos--;
        }
        return kernel_pos;
    }

    long user_pos = kernel_pos;

    if (stream->size > 0) {
        if (stream->size > stream->pos) {
            size_t unread = stream->size - stream->pos;
            if (unread > (size_t)LONG_MAX) {
                errno = EOVERFLOW;
                return -1;
            }
            user_pos -= (long)unread;
        }
        
        if (stream->ungetc_buf != -1) {
            user_pos--;
        }
    }
    else if (stream->flags & _FILE_DIRTY) {
        if (stream->pos > (size_t)LONG_MAX ||
            kernel_pos > LONG_MAX - (long)stream->pos) {
            errno = EOVERFLOW;
            return -1;
        }
        user_pos += (long)stream->pos;
    }

    return user_pos;
}


int fseek(FILE *stream, long offset, int whence) {
    if (!stream)
        return -1;

    stream->ungetc_buf = -1;

    if (!(stream->flags & _FILE_UNBUF)) {
        if (stream->flags & _FILE_DIRTY) {
            if (fflush(stream) != 0) {
                return -1;
            }
        } 
        else if (stream->size > stream->pos) {
            if (__discard_read_buffer(stream) != 0) {
                return -1;
            }
        }
    }

    long res = lseek(stream->fd, offset, whence);

    if (res < 0)
        return -1;

    stream->pos = 0;
    stream->size = 0;
    
    stream->flags &= ~_FILE_EOF;
    return 0;
}

