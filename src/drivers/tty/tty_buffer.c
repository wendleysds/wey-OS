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

int tty_buffer_write(struct tty_struct *tty, const u8 *buffer, size_t count) {
	if (!tty || !buffer || count == 0) {
		return -EINVAL;
	}

	size_t written = 0;
	struct tty_buffer *new_chunk = NULL;

	spin_lock(&tty->buffer.lock);

	while (written < count) {
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
		size_t to_write = count - written;
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

	return (int)written;
}

int tty_buffer_read(struct tty_struct *tty, u8 *buffer, size_t count) {
	if (!tty || !buffer || count == 0) {
		return -EINVAL;
	}

	size_t read_bytes = 0;
	spin_lock(&tty->buffer.lock);

	while (read_bytes < count) {
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

		size_t to_read = count - read_bytes;
		if (to_read > available) {
			to_read = available;
		}

		memcpy(&buffer[read_bytes], &buf->data[buf->read_pos], to_read);
		buf->read_pos += to_read;
		read_bytes += to_read;
	}

	spin_unlock(&tty->buffer.lock);
	return (int)read_bytes;
}

static int __init tty_buffer_init(void) {
	buffers_cache = NULL;
	spinlock_init(&lock);
	return OK;
}

fs_initcall(tty_buffer_init);