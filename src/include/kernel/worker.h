#ifndef _WORKER_H
#define _WORKER_H

#include <lib/list.h>
#include <kernel/wait.h>
#include <stdbool.h>

#define INIT_WORK(_ptr, _func) \
	do { \
		INIT_LIST_HEAD(&((_ptr)->list)); \
		atomic_set(&((_ptr)->data), 0); \
		(_ptr)->func = _func; \
	} while(0)

struct work_struct;

typedef void (*work_func_t)(struct work_struct *work);

struct work_struct {
	atomic_t data;
	work_func_t func;
	struct list_head list;
};

struct work_queue {
	pid_t thread_pid;
	spinlock_t lock;
	int stopping;

	struct wait_queue_head wait;
	struct list_head works;
};

struct work_queue *alloc_work_queue(const char *name);
int queue_work(struct work_queue *wq, struct work_struct *work);
void destroy_work_queue(struct work_queue *wq);

#endif
