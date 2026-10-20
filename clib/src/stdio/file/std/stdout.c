#include "../../internal.h"
#include <unistd.h>

static struct file _stdout = {
    .fd = STDOUT_FILENO,
    .flags = _FILE_WRITE | _FILE_UNBUF
};

struct file *stdout = &_stdout;