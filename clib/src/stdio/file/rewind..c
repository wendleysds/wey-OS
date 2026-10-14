#include "../internal.h"

void rewind(struct file *stream){
    if (!stream) return;

    stream->ungetc_buf = -1;

    fseek(stream, 0, SEEK_SET);

    stream->flags &= ~_FILE_EOF;
    stream->flags &= ~_FILE_ERR;
}
