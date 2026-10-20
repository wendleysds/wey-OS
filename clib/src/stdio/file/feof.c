#include "../internal.h"

int feof(struct file *stream){
    if (!stream) return 0;
    return (stream->flags & _FILE_EOF);
}