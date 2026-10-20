#include "../internal.h"

int ferror(struct file *stream){
    if (!stream) return 0;
    return (stream->flags & _FILE_ERR);
}