#include "../../internal.h"
#include <unistd.h>

static struct file _stdin = {
    .fd = STDIN_FILENO,
    .flags = _FILE_READ | _FILE_UNBUF
};

struct file *stdin = &_stdin;