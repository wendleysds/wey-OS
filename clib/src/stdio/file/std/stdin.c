#include "../../internal.h"
#include <unistd.h>

static struct file _stdin = {
    .fd = STDIN_FILENO,
    .ungetc_buf = -1,
    .flags = _FILE_READ | _FILE_UNBUF
};

struct file *stdin = &_stdin;