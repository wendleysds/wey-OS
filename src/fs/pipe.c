#include <kernel/sched.h>
#include <kernel/signal.h>
#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <kernel/wait.h>
#include <fs/vfs.h>
#include <lib/string.h>
#include <mm/kheap.h>
#include <def/errno.h>
#include <stdint.h>

#define PIPE_SIZE 4096
#define PIPE_BUF  PIPE_SIZE
#define MIN(a, b) ((a) < (b) ? (a) : (b))

_Static_assert((PIPE_SIZE & (PIPE_SIZE - 1)) == 0, "PIPE_SIZE must be a power of two");

struct pipe {
	u8     buf[PIPE_SIZE];
	size_t head;            /* next byte to write */
	size_t tail;            /* next byte to read  */
	size_t count;           /* bytes stored       */

	int readers;            /* open read-end files  */
	int writers;            /* open write-end files */

	spinlock_t lock;        /* protects everything above */
	struct wait_queue_head rd_wait;
	struct wait_queue_head wr_wait;
};

static size_t ring_read(struct pipe *p, u8 *dst, size_t want){
	size_t n     = MIN(want, p->count);
	size_t first = MIN(n, PIPE_SIZE - p->tail);

	memcpy(dst, p->buf + p->tail, first);
	memcpy(dst + first, p->buf, n - first);

	p->tail   = (p->tail + n) & (PIPE_SIZE - 1);
	p->count -= n;
	return n;
}

static size_t ring_write(struct pipe *p, const u8 *src, size_t want){
	size_t n     = MIN(want, PIPE_SIZE - p->count);
	size_t first = MIN(n, PIPE_SIZE - p->head);

	memcpy(p->buf + p->head, src, first);
	memcpy(p->buf, src + first, n - first);

	p->head   = (p->head + n) & (PIPE_SIZE - 1);
	p->count += n;
	return n;
}

/* Called with p->lock held; returns with it released. */
static void pipe_sleep(struct pipe *p){
	task_sleep_interruptible(current);
	spin_unlock(&p->lock);
	schedule();
}

static ssize_t pipe_read(struct file *file, void *buffer, size_t count, off_t *offset){
	struct pipe *p = file->private_data;
	struct wait_queue_entry wait;
	ssize_t ret;

	if (count == 0)
		return 0;

	wait_queue_entry_init(&wait, current, task_default_wakeup);
	wait_queue_add(&p->rd_wait, &wait);

	for (;;) {
		spin_lock(&p->lock);

		if (p->count > 0) {
			ret = (ssize_t)ring_read(p, buffer, count);
			spin_unlock(&p->lock);
			wake_up_all(&p->wr_wait);
			break;
		}

		if (p->writers == 0) {  /* drained and nobody can write: EOF */
			spin_unlock(&p->lock);
			ret = 0;
			break;
		}

		if (file->flags & O_NONBLOCK) {
			spin_unlock(&p->lock);
			ret = -EAGAIN;
			break;
		}

		if (has_signal_pending(current)) {
			spin_unlock(&p->lock);
			ret = -EINTR;
			break;
		}

		pipe_sleep(p);
	}

	wait_queue_remove(&p->rd_wait, &wait);
	return ret;
}

static ssize_t pipe_write(struct file *file, const void *buffer, size_t count,
			  off_t *offset){
	struct pipe *p = file->private_data;
	const u8 *src = buffer;
	struct wait_queue_entry wait;
	size_t total = 0;
	ssize_t ret = 0;
	int broken = 0;

	if (count == 0)
		return 0;

	wait_queue_entry_init(&wait, current, task_default_wakeup);
	wait_queue_add(&p->wr_wait, &wait);

	while (total < count) {
		/* Small writes are atomic: wait until the whole thing fits. */
		size_t need = (count <= PIPE_BUF) ? count : 1;

		spin_lock(&p->lock);

		if (p->readers == 0) {
			spin_unlock(&p->lock);
			broken = 1;
			ret = total ? (ssize_t)total : -EPIPE;
			break;
		}

		if (PIPE_SIZE - p->count >= need) {
			total += ring_write(p, src + total, count - total);
			spin_unlock(&p->lock);
			wake_up_all(&p->rd_wait);
			continue;
		}

		if (file->flags & O_NONBLOCK) {
			spin_unlock(&p->lock);
			ret = total ? (ssize_t)total : -EAGAIN;
			break;
		}

		if (has_signal_pending(current)) {
			spin_unlock(&p->lock);
			ret = total ? (ssize_t)total : -EINTR;
			break;
		}

		pipe_sleep(p);
	}

	if (total == count)
		ret = (ssize_t)total;

	wait_queue_remove(&p->wr_wait, &wait);

	if (broken)
		send_signal(current, SIGPIPE);

	return ret;
}

/* Wrong-direction operations. */
static ssize_t pipe_bad_read(struct file *f, void *b, size_t c, off_t *o){
	return -EBADF;
}

static ssize_t pipe_bad_write(struct file *f, const void *b, size_t c, off_t *o){
	return -EBADF;
}

static off_t pipe_lseek(struct file *file, off_t offset, int whence){
	return -ESPIPE;
}

/*
 * Wake the other side *before* dropping the lock: once the lock is gone the
 * peer's release may see `dead` and free the pipe under our feet.
 */
static int pipe_read_release(struct inode *inode, struct file *file){
	struct pipe *p = file->private_data;
	int dead;

	spin_lock(&p->lock);
	p->readers--;
	wake_up_all(&p->wr_wait); /* writers must now see EPIPE */
	dead = !p->readers && !p->writers;
	spin_unlock(&p->lock);

	if (dead)
		kfree(p);
	return 0;
}

static int pipe_write_release(struct inode *inode, struct file *file){
	struct pipe *p = file->private_data;
	int dead;

	spin_lock(&p->lock);
	p->writers--;
	wake_up_all(&p->rd_wait); /* readers must now see EOF */
	dead = !p->readers && !p->writers;
	spin_unlock(&p->lock);

	if (dead)
		kfree(p);
	return 0;
}

static const struct file_operations pipe_read_fops = {
	.read    = pipe_read,
	.write   = pipe_bad_write,
	.lseek   = pipe_lseek,
	.release = pipe_read_release,
};

static const struct file_operations pipe_write_fops = {
	.read    = pipe_bad_read,
	.write   = pipe_write,
	.lseek   = pipe_lseek,
	.release = pipe_write_release,
};

static struct pipe *pipe_alloc(void){
	struct pipe *p = kzalloc(sizeof(*p));
	if (!p)
		return NULL;

	spinlock_init(&p->lock);
	wait_queue_head_init(&p->rd_wait);
	wait_queue_head_init(&p->wr_wait);
	p->readers = 1;
	p->writers = 1;
	return p;
}

static struct file *pipe_file_new(struct pipe *p, unsigned int accmode, unsigned int flags, const struct file_operations *fops){
	struct file *f = kzalloc(sizeof(*f));
	if (!f)
		return NULL;

	spinlock_init(&f->lock);
	atomic_set(&f->refcount, 1);
	f->inode        = NULL;
	f->private_data = p;
	f->flags        = accmode | flags;
	f->f_op         = fops;
	return f;
}

static long do_pipe2(int __user *pipefd, unsigned int flags){
	struct pipe *p;
	struct file *rf, *wf;
	int fds[2];
	long ret;

	if (flags & ~(O_NONBLOCK | O_CLOEXEC))
		return -EINVAL;

	p = pipe_alloc();
	if (!p)
		return -ENOMEM;

	rf = pipe_file_new(p, O_RDONLY, flags, &pipe_read_fops);
	if (!rf) {
		kfree(p);
		return -ENOMEM;
	}

	wf = pipe_file_new(p, O_WRONLY, flags, &pipe_write_fops);
	if (!wf) {
		kfree(rf); /* never installed: no release() */
		kfree(p);
		return -ENOMEM;
	}

	/*
	 * From here on both files own the pipe. Every error path ends in the
	 * single cleanup below: dropping our creation reference runs release()
	 * on each end, and the last one frees the pipe.
	 */
	fds[0] = task_add_file(current, rf);
	if (fds[0] < 0) {
		ret = fds[0];
		goto out_put;
	}

	fds[1] = task_add_file(current, wf);
	if (fds[1] < 0) {
		ret = fds[1];
		task_remove_file(current, fds[0]);
		goto out_put;
	}

	if (copy_to_user(pipefd, fds, sizeof(fds))) {
		task_remove_file(current, fds[0]);
		task_remove_file(current, fds[1]);
		ret = -EFAULT;
		goto out_put;
	}

	ret = 0;

out_put:
	file_put(rf); /* drop creation ref; the fd table keeps its own */
	file_put(wf);
	return ret;
}

SYSCALL_DEFINE2(pipe2, int __user *, pipefd, unsigned int, flags){
	return do_pipe2(pipefd, flags);
}

SYSCALL_DEFINE1(pipe, int __user *, pipefd){
	return do_pipe2(pipefd, 0);
}
