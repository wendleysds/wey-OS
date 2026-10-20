#include "../internal.h"

int fileno(struct file *stream){
    if (!stream) return -1;
    return stream->fd;
}