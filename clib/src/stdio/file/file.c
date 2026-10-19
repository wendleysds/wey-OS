#include "../internal.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static struct file *opened_files[FOPEN_MAX];
static struct file file_pool[FOPEN_MAX];

static struct file _stdin = {
    .fd = STDIN_FILENO,
    .flags = _FILE_READ | _FILE_UNBUF
};

static struct file _stdout = {
    .fd = STDOUT_FILENO,
    .flags = _FILE_WRITE | _FILE_UNBUF
};

static struct file _stderr = {
    .fd = STDERR_FILENO,
    .flags = _FILE_WRITE | _FILE_UNBUF
};

struct file *stdin = &_stdin;
struct file *stdout = &_stdout;
struct file *stderr = &_stderr;

#define stdin stdin
#define stdout stdout
#define stderr stderr

static int __mode_to_file_flags(const char *mode){
    int flags = 0;
    
    if (strcmp(mode, "r") == 0) 
        flags = _FILE_READ;
    else if (strcmp(mode, "w") == 0) 
        flags = _FILE_WRITE;
    else if (strcmp(mode, "a") == 0) 
        flags = _FILE_WRITE;
    else if (strcmp(mode, "r+") == 0) 
        flags = _FILE_READ | _FILE_WRITE;
    else if (strcmp(mode, "w+") == 0) 
        flags = _FILE_READ | _FILE_WRITE;
    else if (strcmp(mode, "a+") == 0) 
        flags = _FILE_READ | _FILE_WRITE;
    else
        return -1;
    
    return flags;
}

static int __mode_to_open_flags(const char *mode)
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


int __file_alloc(int fd, const char *mode) {
    if (fd < 0) return -1;
    if (!mode) return -1;

    struct file *f = NULL;
    for(int i = 0; i < FOPEN_MAX; i++) {
        if (!file_pool[i].fd) {
            f = &file_pool[i];
            break;
        }
    }
    if (!f) return -1;

    memset(f, 0, sizeof(struct file));

    int flags = __mode_to_file_flags(mode);
    if (flags == -1) return -1;

    f->flags = flags | _FILE_UNBUF;
    f->buffer = NULL;

    f->fd = fd;
    f->capacity = 0;
    f->size = 0;
    f->pos = 0;

    for(int i = 0; i < FOPEN_MAX; i++) {
        if (!opened_files[i]) {
            opened_files[i] = f;
            return i;
        }
    }

    return -1;
}

struct file *fdopen(int fd, const char *mode){
    int index = __file_alloc(fd, mode);
    if (index == -1) return NULL;
    return &file_pool[index];
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

            /*if(stream->buffer) {
                free(stream->buffer);
            }*/
            
            close(stream->fd);
            break;
        }
    }

    return 0;
}

void rewind(struct file *stream){
    if (!stream) return;

    stream->ungetc_buf = -1;

    fseek(stream, 0, SEEK_SET);

    stream->flags &= ~_FILE_EOF;
    stream->flags &= ~_FILE_ERR;
}

int feof(struct file *stream){
    if (!stream) return 0;
    return (stream->flags & _FILE_EOF);
}

int ferror(struct file *stream){
    if (!stream) return 0;
    return (stream->flags & _FILE_ERR);
}

void clearerr(struct file *stream){
    if (!stream) return;
    stream->flags &= ~_FILE_EOF;
    stream->flags &= ~_FILE_ERR;
}

int fileno(struct file *stream){
    if (!stream) return -1;
    return stream->fd;
}