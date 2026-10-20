#include "../../internal.h"
#include <unistd.h>

static struct file _stdout = {
    .fd = STDOUT_FILENO,
    .ungetc_buf = -1,
    .flags = _FILE_WRITE | _FILE_UNBUF
};

struct file *stdout = &_stdout;