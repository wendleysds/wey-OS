#include <kernel/worker.h>
#include <kernel/fork.h>
#include <def/errno.h>
#include <mm/kheap.h>

static int worker_thread(void *arg)
{
	struct work_queue *wq = arg;

	while (1) {
		wait_event(&wq->wait, !list_empty(&wq->works));

		spin_lock(&wq->lock);

		struct work_struct *work = list_first_entry(&wq->works, struct work_struct, list);
		list_remove(&work->list);

		spin_unlock(&wq->lock);

		work->func(work);
	}
}

int queue_work(struct work_queue *wq, struct work_struct *work){
	INIT_LIST_HEAD(&work->list);

	spin_lock(&wq->lock);
	if (wq->stopping) {
		spin_unlock(&wq->lock);
		return -ESHUTDOWN;
	}

	list_add(&wq->works, &work->list);
	spin_unlock(&wq->lock);

	return SUCCESS;
}

struct work_queue *alloc_work_queue(const char *name){
	struct work_queue* queue = kmalloc(sizeof(struct work_queue));
	if(queue){
		INIT_LIST_HEAD(&queue->works);
		spinlock_init(&queue->lock);
		wait_queue_head_init(&queue->wait);

		pid_t res = kernel_thread(worker_thread, name, queue);
		if(res < 0){
			kfree(queue);
			return NULL;
		}

		queue->thread_pid = res;
	}

	return queue;
}

void destroy_work_queue(struct work_queue *wq){
	spin_lock(&wq->lock);
	wq->stopping = 1;
	spin_unlock(&wq->lock);
}