#ifndef _WAIT_H
#define _WAIT_H

#include <sync/spinlock.h>
#include <lib/list.h>

struct wait_queue_entry;

typedef int (*wait_queue_func_t)(struct wait_queue_entry*);

struct wait_queue_entry {
	void *private;
	wait_queue_func_t callback;
	struct list_head nodes;
};

struct wait_queue_head {
	spinlock_t lock;
	struct list_head entries;
};

static inline void wait_queue_head_init(struct wait_queue_head *head) {
    spinlock_init(&head->lock);
    INIT_LIST_HEAD(&head->entries);
}

static inline void wait_queue_entry_init(
    struct wait_queue_entry *entry,
    void *private,
    wait_queue_func_t callback)
{
    entry->private = private;
    entry->callback = callback;
    INIT_LIST_HEAD(&entry->nodes);
}

static inline int wait_queue_active(struct wait_queue_head *queue) {
	return !list_empty(&queue->entries);
}

void wait_queue_add(struct wait_queue_head *head, struct wait_queue_entry *new_entry);
void wait_queue_remove(struct wait_queue_head *head, struct wait_queue_entry *entry);

int __wake_up(struct wait_queue_head *head, int count);
#define wake_up(x)        __wake_up(x, 1)
#define wake_up_nr(x, nr) __wake_up(x, nr)
#define wake_up_all(x)    __wake_up(x, 0)

#endif
