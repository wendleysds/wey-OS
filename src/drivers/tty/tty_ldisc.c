#include <device/tty.h>
#include <def/errno.h>
#include <def/config.h>
#include <kernel/sched.h>
#include <kernel/printk.h>
#include <mm/kheap.h>

struct tty_ldisc_data {
	spinlock_t lock;
	size_t head;
	size_t tail;
	size_t count;
	u8 buffer[TTY_BUFFER_LDISC_SIZE];
};

extern const struct tty_ldisc_ops tty_ldisc_ops;

static int tty_ldisc_open(struct tty_struct* tty){
	if(!tty){
		return -EINVAL;
	}

	if(tty->ldisc){
		return 0;
	}

	struct tty_ldisc* ldisc = kmalloc(sizeof(struct tty_ldisc));
	if(!ldisc){
		return -ENOMEM;
	}

	struct tty_ldisc_data* data = kzalloc(sizeof(*data));
	if(!data){
		kfree(ldisc);
		return -ENOMEM;
	}

	spinlock_init(&data->lock);
	data->head = 0;
	data->tail = 0;
	data->count = 0;

	ldisc->ops = &tty_ldisc_ops;
	ldisc->tty = tty;

	tty->ldisc = ldisc;
	tty->ldisc_data = data;

	return 0;
}

static int tty_ldisc_close(struct tty_struct* tty){
	if(!tty || !tty->ldisc){
		return 0;
	}

	kfree(tty->ldisc);

	if(tty->ldisc_data)
		kfree(tty->ldisc_data);

	tty->ldisc = NULL;
	tty->ldisc_data = NULL;
	
	return 0;
}

static int tty_ldisc_read(struct tty_struct* tty, u8 *buffer, size_t len){
	if(!tty || !buffer || len == 0){
		return 0;
	}

	if(!tty->ldisc || !tty->ldisc_data){
		return -ENODEV;
	}

	struct tty_ldisc_data* state = tty->ldisc_data;
	struct wait_queue_entry wait;
	wait_queue_entry_init(&wait, current, task_default_wakeup);

	size_t bytes_read = 0;
	wait_queue_add(&tty->read_waiters, &wait);
	
	while(len > 0){
		unsigned long flags;
		spin_lock_irqsave(&state->lock, &flags);

		if(state->count == 0){
			spin_unlock_irqrestore(&state->lock, &flags);
			if(bytes_read > 0){
				break;
			}
			sleep_current();
			continue;
		}

		int line_done = 0;
		while(state->count > 0 && len > 0){
			u8 ch = state->buffer[state->tail];
			state->tail = (state->tail + 1) % TTY_BUFFER_LDISC_SIZE;
			state->count--;

			buffer[bytes_read++] = ch;
			len--;

			if(ch == '\n' || ch == '\r'){
				line_done = 1;
				break;
			}
		}

		spin_unlock_irqrestore(&state->lock, &flags);

		if(line_done || bytes_read > 0){
			break;
		}
	}

	wait_queue_remove(&tty->read_waiters, &wait);
	return bytes_read;
}

int tty_ldisc_receive_buf(struct tty_struct* tty, const u8* data, size_t len){
	if(!tty || !data || len == 0){
		return 0;
	}

	if(!tty->ldisc || !tty->ldisc_data){
		return -ENODEV;
	}

	unsigned long flags;
	int wake_all = 0;

	struct tty_ldisc_data* state = tty->ldisc_data;

	spin_lock_irqsave(&state->lock, &flags);
	for(size_t i = 0; i < len; i++){
		u8 ch = data[i];

		if(ch == '\b'){
			if(state->count > 0){
				state->head = (state->head + TTY_BUFFER_LDISC_SIZE - 1) % TTY_BUFFER_LDISC_SIZE;
				state->count--;
				state->buffer[state->head] = '\0';
				if(tty->ops && tty->ops->write){
					tty->ops->write(tty, (u8*)"\b \b", 3);
				}
			}

			continue;
		}

		if(ch == '\n' || ch == '\r') {
			wake_all = 1;
			goto append;
		}

		if(ch >= 0x01 && ch <= 0x1A){
			u8 tmp[3] = { '^', 'A' + ch - 1, '\n' };
			state->head = 0;
			state->tail = 0;
			state->count = 0;
			if(tty->ops && tty->ops->write){
				tty->ops->write(tty, tmp, 3);
			}
			continue;
		}

append:
		if(state->count < TTY_BUFFER_LDISC_SIZE){
			state->buffer[state->head] = ch;
			state->head = (state->head + 1) % TTY_BUFFER_LDISC_SIZE;
			state->count++;
			if(tty->ops && tty->ops->write){
				tty->ops->write(tty, &ch, 1);
			}
		}
	}
	spin_unlock_irqrestore(&state->lock, &flags);

	if(wake_all)
		wake_up(&tty->read_waiters);

	return 0;
}

const struct tty_ldisc_ops tty_ldisc_ops = {
	.open = tty_ldisc_open,
	.close = tty_ldisc_close,
	.read = tty_ldisc_read,

	.receive_buf = tty_ldisc_receive_buf,
};

void tty_receive_char(struct tty_struct* tty, char ch){
	if(!tty || !tty->ldisc || !tty->ldisc->ops || !tty->ldisc->ops->receive_buf){
		return;
	}

	tty->ldisc->ops->receive_buf(tty, (u8*)&ch, 1);
}
