#include "../internal.h"

void clearerr(struct file *stream){
    if (!stream) return;
    stream->flags &= ~_FILE_EOF;
    stream->flags &= ~_FILE_ERR;
}