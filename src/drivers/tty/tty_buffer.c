#include <device/tty.h>
#include <kernel/init.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/errno.h>
#include <mm/kheap.h>

static struct tty_buffer *buffers_cache = NULL;
static spinlock_t lock;

static struct tty_buffer *tty_alloc_buffer(void) {
	struct tty_buffer *buffer;

	spin_lock(&lock);
	if(buffers_cache) {
		buffer = buffers_cache;
		buffers_cache = buffer->next;
		spin_unlock(&lock);
		return buffer;
	}
	spin_unlock(&lock);

	buffer = kzalloc(sizeof(struct tty_buffer));

	return buffer;
}

static void tty_free_buffer(struct tty_buffer *buffer) {
	memset(buffer, 0x0, sizeof(struct tty_buffer));

	spin_lock(&lock);
	buffer->next = buffers_cache;
	buffers_cache = buffer;
	spin_unlock(&lock);
}

static struct tty_buffer *tty_buffer_alloc_chunk(void) {
	struct tty_buffer *buffer = tty_alloc_buffer();
	if (!buffer) return NULL;

	buffer->data = kzalloc(TTY_BUFFER_CHUNK_SIZE);
	if (!buffer->data) {
		tty_free_buffer(buffer);
		return NULL;
	}

	buffer->size = TTY_BUFFER_CHUNK_SIZE;
	buffer->read_pos = 0;
	buffer->write_pos = 0;
	buffer->next = NULL;
	return buffer;
}

static void tty_buffer_free_chunk(struct tty_buffer *buffer) {
	kfree(buffer->data);
	tty_free_buffer(buffer);
}

static void flush_to_ldisc(struct tty_struct* tty){
	if(!tty->ldisc || !tty->ldisc->ops->receive_buf){
		return;
	}

	spin_lock(&tty->buffer.lock);

	struct tty_buffer *buf = tty->buffer.head;

	size_t available = buf->write_pos - buf->read_pos;
	if(!available){
		spin_unlock(&tty->buffer.lock);
		return;
	}

	while (1) {
		struct tty_buffer *buf = tty->buffer.head;
		if (!buf) break;

		size_t available = buf->write_pos - buf->read_pos;
		
		if (available == 0) {
			if (buf->next) {
				struct tty_buffer *next = buf->next;
				tty->buffer.head = next;

				spin_unlock(&tty->buffer.lock);
				tty_buffer_free_chunk(buf);
				spin_lock(&tty->buffer.lock);
				continue;
			} else {
				buf->read_pos = 0;
				buf->write_pos = 0;
				break;
			}
		}

		// Dispatch 
		tty->ldisc->ops->receive_buf(tty, &buf->data[buf->read_pos], available);
		buf->read_pos += available;
	}
	
	spin_unlock(&tty->buffer.lock);
}

int tty_bufhead_init(struct tty_bufhead* bufhead){
	memset(bufhead, 0x0, sizeof(struct tty_bufhead));
	spinlock_init(&bufhead->lock);
	return SUCCESS;
}

// IRQ -> tty_receive_buf -> [tty_worker]
// [tty_worker] -> flush_to_ldisc -> tty_ldisc_receive_buf
int tty_receive_buf(struct tty_struct* tty, const u8* buffer, size_t len){
	if (!tty || !buffer || len == 0) {
		return -EINVAL;
	}

	int written = 0;
	struct tty_buffer *new_chunk = NULL;

	spin_lock(&tty->buffer.lock);

	while (written < len) {
		struct tty_buffer *buf = tty->buffer.tail;

		if (!buf || buf->write_pos == buf->size) {
			if (!new_chunk) {
				spin_unlock(&tty->buffer.lock);
				new_chunk = tty_buffer_alloc_chunk();
				if (!new_chunk) {
					return written > 0 ? (int)written : -ENOMEM;
				}

				spin_lock(&tty->buffer.lock);
				continue; 
			}

			if (!tty->buffer.head) {
				tty->buffer.head = new_chunk;
				tty->buffer.tail = new_chunk;
			} else {
				tty->buffer.tail->next = new_chunk;
				tty->buffer.tail = new_chunk;
			}

			buf = new_chunk;
			new_chunk = NULL; // Consumed
		}

		size_t available = buf->size - buf->write_pos;
		size_t to_write = len - written;
		if (to_write > available) {
			to_write = available;
		}

		memcpy(&buf->data[buf->write_pos], &buffer[written], to_write);
		buf->write_pos += to_write;
		written += to_write;
	}

	spin_unlock(&tty->buffer.lock);

	if (new_chunk) {
		tty_buffer_free_chunk(new_chunk);
	}

	//TODO: Add tty_worker latter

	flush_to_ldisc(tty);

	return written;
}

static int __init tty_buffer_init(void) {
	buffers_cache = NULL;
	spinlock_init(&lock);
	return OK;
}

fs_initcall(tty_buffer_init);