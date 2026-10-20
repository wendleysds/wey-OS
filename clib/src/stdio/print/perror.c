#include <stdio.h>
#include <errno.h>

int perror(const char *s) {
    int count = 0;

    if (s && *s) {
        count += fprintf(stderr, "%s: ", s);
    }

    count += fprintf(stderr, "%s\n", strerror(errno));
    return count;
}
