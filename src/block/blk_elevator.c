#include <device/blkdev.h>

static void fifo_insert(struct request_queue *q, struct request *rq){
	rq->next = NULL;

	if (!q->head)
		q->head = rq;
	else
		q->tail->next = rq;

	q->tail = rq;
}

static struct request *fifo_dispatch(struct request_queue *q){
	struct request *rq = q->head;

	if (!rq)
		return NULL;

	q->head = rq->next;
	if (!q->head)
		q->tail = NULL;

	return rq;
}

struct elevator_ops fifo_elevator = {
	.insert = fifo_insert,
	.dispatch = fifo_dispatch,
};
