#include <lib/list.h>
#include <wey/blkdev.h>
#include <mm/kheap.h>
#include <def/err.h>
#include <def/config.h>
#include <stddef.h>

static struct request* bio_to_request(struct bio *bio){
	if (bio->sector + bio->len > bio->bdev->nr_sectors){
		return ERR_PTR(INVALID_ARG);
	}

	struct request* rq = kmalloc(sizeof(struct request));
	if(!rq){
		return ERR_PTR(NO_MEMORY);
		
	}

	memset(rq, 0x0, sizeof(struct request));
	uint16_t secsize = bio->bdev->disk->sec_size;
	rq->sector = bio->sector + bio->bdev->start_sector;
	rq->nr_sectors = ALIGN(bio->len, secsize) / secsize;
	rq->op = bio->op;
	rq->q = bio->bdev->queue;

	if(rq->bio_list){
		bio->next = rq->bio_list;
	}else{
		bio->next = NULL;
	}

	rq->bio_list = bio;
	return rq;
}

static void bio_endio(struct bio *bio){
	if (bio->end_io)
		bio->end_io(bio);
}

void blk_run_queue(struct request_queue *q){
	struct request *rq;

	spin_lock(&q->lock);
	rq = q->elevator->dispatch(q);
	spin_unlock(&q->lock);

	if (rq){
		blk_complete_request(
			rq,
			q->bdev->disk->ops->submit_request(q->bdev, rq)
		);
	}
}

int blk_submit_bio(struct blkdev *bdev, struct bio *bio){
	bio->bdev = bdev;
	struct request *rq = bio_to_request(bio);
	if(IS_ERR_VALUE(rq)){
		return PTR_ERR(rq);
	}

	spin_lock(&bdev->queue->lock);
	bdev->queue->elevator->insert(bdev->queue, rq);
	spin_unlock(&bdev->queue->lock);

	blk_run_queue(bdev->queue);

	return SUCCESS;
}

void blk_complete_request(struct request *rq, int status){
	struct bio *bio;

	rq->status = status;
	bio = rq->bio_list;

	while (bio) {
		bio->status = status;
		bio_endio(bio);
		bio = bio->next;
	}

	struct request_queue* q = rq->q;
	kfree(rq);
	blk_run_queue(q);
}
