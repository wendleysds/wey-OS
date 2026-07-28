#include "kernel/wait.h"
#include <device/tty.h>
#include <device/terminal.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <mm/kheap.h>
#include <def/errno.h>
#include <lib/string.h>

extern const struct tty_ldisc_ops tty_ldisc_ops;

static int tty_task_awake(struct wait_queue_entry* entry){
	struct task* task = entry->private;
	task_wakeup(task);

	return 1;
}

static int tty_ldisc_open(struct tty_struct* tty){
	if(!tty){
		return -EINVAL;
	}

	if(tty->ldisc){
		return 0;
	}

	struct tty_line_discipline_state* state = kzalloc(sizeof(*state));
	if(!state){
		return -ENOMEM;
	}

	memset(state, 0, sizeof(*state));
	state->ldisc.ops = &tty_ldisc_ops;
	state->ldisc.data = state;
	tty->ldisc = &state->ldisc;
	tty->private = state;
	return 0;
}

static int tty_ldisc_close(struct tty_struct* tty){
	if(!tty || !tty->private){
		return 0;
	}

	kfree(tty->private);
	tty->private = NULL;
	tty->ldisc = NULL;
	return 0;
}

static int tty_ldisc_read(struct tty_struct* tty, char *buffer, size_t len){
	if(!tty || !buffer || len == 0){
		return 0;
	}

	if(!tty->ldisc || !tty->ldisc->data){
		return -ENODEV;
	}

	struct tty_line_discipline_state* state = tty->ldisc->data;
	struct wait_queue_entry wait;
	wait_queue_entry_init(&wait, current, tty_task_awake);

	size_t bytes_read = 0;
	wait_queue_add(&tty->read_waiters, &wait);
	
	while(len > 0){
		size_t available = state->len;
		
		/* Copy available data from buffer */
		for(size_t idx = 0; idx < available && len > 0; idx++, bytes_read++, len--){
			char ch = state->buffer[idx];
			buffer[bytes_read] = ch;

			if(ch == '\n' || ch == '\r'){
				state->len = 0;
				wait_queue_remove(&tty->read_waiters, &wait);
				return bytes_read;
			}
		}

		state->len = 0;
		task_sleep(current);
		schedule();
	}

	wait_queue_remove(&tty->read_waiters, &wait);
	return bytes_read;
}

static int tty_ldisc_receive_buf(struct tty_struct* tty, const char* data, size_t len){
	if(!tty || !data || len == 0){
		return 0;
	}

	if(!tty->ldisc || !tty->ldisc->data){
		return -ENODEV;
	}

	unsigned long flags;
	int wake_all = 0;

	struct tty_line_discipline_state* state = tty->ldisc->data;

	spin_lock_irqsave(&state->lock, &flags);
	for(size_t i = 0; i < len; i++){
		char ch = data[i];

		if(ch == '\b'){
			if(state->len > 0){
				state->len--;
				state->buffer[state->len] = '\0';
				if(tty->ops && tty->ops->write){
					tty->ops->write(tty, "\b \b", 3);
				}
			}

			continue;
		}

		if(ch == '\n' || ch == '\r') {
			wake_all = 1;
			goto append;
		}

		if(ch >= 0x01 && ch <= 0x1A){
			char tmp[3] = { '^', 'A' + ch - 1, '\n' };
			state->len = 0;
			if(tty->ops && tty->ops->write){
				tty->ops->write(tty, tmp, 3);
			}
			continue;
		}

append:
		if(state->len < sizeof(state->buffer) - 1){
			state->buffer[state->len++] = ch;
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

	tty->ldisc->ops->receive_buf(tty, &ch, 1);
}
