#include "../../internal.h"
#include <unistd.h>

static struct file _stderr = {
    .fd = STDERR_FILENO,
    .ungetc_buf = -1,
    .flags = _FILE_WRITE | _FILE_UNBUF
};

struct file *stderr = &_stderr;