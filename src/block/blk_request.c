#include <device/blkdev.h>
#include <mm/kheap.h>
#include <def/errno.h>

static struct request* bio_to_request(struct bio *bio){
	if (!bio || !bio->bdev || !bio->bdev->disk){
		return ERR_PTR(-EINVAL);
	}

	const sector_t start   = bio->bdev->start_sector;
	const sector_t nr_sectors = bio->nr_sectors;
	const sector_t capacity = bio->bdev->nr_sectors;

	if (bio->sector < start) {
		return ERR_PTR(-EINVAL);
	}

	const sector_t relative_sector = bio->sector - start;

	if (relative_sector + nr_sectors > capacity) {
		return ERR_PTR(-ERANGE);
	}

	struct request *rq = kzalloc(sizeof(*rq));
	if (!rq){
		return ERR_PTR(-ENOMEM);
	}

	rq->sector     = bio->sector;
	rq->nr_sectors = nr_sectors;
	rq->op         = bio->op;
	rq->q          = bio->bdev->queue;
	rq->bio_list   = bio;
	bio->next = NULL;

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
