#include "../internal.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static struct file *opened_files[FOPEN_MAX];

int __mode_to_open_flags(const char *mode)
{
    if (!mode)
        return -1;

    int flags;

    switch (*mode++) {
    case 'r':
        flags = O_RDONLY;
        break;
    case 'w':
        flags = O_WRONLY | O_CREAT | O_TRUNC;
        break;
    case 'a':
        flags = O_WRONLY | O_CREAT | O_APPEND;
        break;
    default:
        return -1;
    }

    int plus = 0;

    for (; *mode; mode++) {
        if (*mode == '+' && !plus) {
            flags = (flags & ~O_ACCMODE) | O_RDWR;
            plus = 1;
        } else if (*mode == 'b' || *mode == 'e' || *mode == 'x') {
            if (*mode == 'e')
                flags |= O_CLOEXEC;
            else if (*mode == 'x')
                flags |= O_EXCL | O_CREAT;
        } else {
            return -1;
        }
    }

    return flags;
}

int __mode_to_file_flags(const char *mode) {
    int open_flags = __mode_to_open_flags(mode);
    if (open_flags == -1) {
        return -1;
    }

    int acc = open_flags & O_ACCMODE;
    if (acc == O_RDWR) {
        return _FILE_READ | _FILE_WRITE;
    } else if (acc == O_WRONLY) {
        return _FILE_WRITE;
    } else {
        return _FILE_READ;
    }
}

int __file_register(struct file *stream) {
    if (!stream) return -1;
    for (int i = 0; i < FOPEN_MAX; i++) {
        if (opened_files[i] == stream) {
            return i;
        }
    }
    for (int i = 0; i < FOPEN_MAX; i++) {
        if (!opened_files[i]) {
            opened_files[i] = stream;
            return i;
        }
    }
    return -1;
}

static struct file* __file_alloc(int fd, const char *mode) {
    if (fd < 0 || !mode) return NULL;

    struct file *f = malloc(sizeof(struct file));
    if (f) {
        int flags = __mode_to_file_flags(mode);
        if (flags == -1) {
            free(f);
            return NULL;
        }

        memset(f, 0, sizeof(struct file));
        f->flags = flags | _FILE_UNBUF;
        f->buffer = NULL;
        f->fd = fd;
        f->capacity = 0;
        f->size = 0;
        f->pos = 0;
        f->ungetc_buf = -1;

        if (__file_register(f) == -1) {
            free(f);
            return NULL;
        }
    }

    return f;
}

struct file *fdopen(int fd, const char *mode){
    return __file_alloc(fd, mode);
}

struct file *fopen(const char *filename, const char *mode) {
    int open_flags = __mode_to_open_flags(mode);
    if (open_flags == -1) return NULL;

    int fd = open(filename, open_flags);
    if (fd < 0) return NULL;
    return fdopen(fd, mode);
}

int fclose(struct file *stream){
    if (!stream) return EOF;
    for(int i = 0; i < FOPEN_MAX; i++) {
        if (opened_files[i] == stream) {
            opened_files[i] = NULL;

            if(stream->flags & _FILE_DIRTY) {
                fflush(stream);
            }

            if ((stream->flags & _FILE_OWNBUF) && stream->buffer) {
                free(stream->buffer);
                stream->buffer = NULL;
                stream->flags &= ~_FILE_OWNBUF;
            }
            
            close(stream->fd);
            free(stream);
            break;
        }
    }

    return 0;
}

void file_close_all(void){
    for (int i = 0; i < FOPEN_MAX; i++) {
        if (opened_files[i]) {
            fclose(opened_files[i]);
        }
    }
}
