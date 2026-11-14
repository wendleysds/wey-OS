#include <stdlib.h>

struct atexit_node {
    void (*func)(void);
    struct atexit_node *next;
};

static struct atexit_node *atexit_head = NULL;

int atexit(void (*func)(void)) {
    struct atexit_node *new_node = malloc(sizeof(struct atexit_node));
    if (!new_node) {
        return -1;
    }

    new_node->func = func;
    new_node->next = atexit_head;
    atexit_head = new_node;

    return 0;
}

void atexit_run(void) {
    while (atexit_head != NULL) {
        struct atexit_node *cur = atexit_head;
        atexit_head = atexit_head->next;
        cur->func();
        free(cur);
    }
}
