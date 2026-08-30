#include <device/tty.h>
#include <def/errno.h>
#include <def/config.h>
#include <kernel/sched.h>
#include <kernel/printk.h>
#include <mm/kheap.h>
#include <stdbool.h>

struct tty_ldisc_data {
	spinlock_t lock;
	size_t head;
	size_t tail;
	size_t count;
	size_t lines;
	bool eof_pending;
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
	data->lines = 0;
	data->eof_pending = false;

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
	const struct termios *t = &tty->termios;
	struct wait_queue_entry wait;
	wait_queue_entry_init(&wait, current, task_default_wakeup);

	size_t bytes_read = 0;
	wait_queue_add(&tty->read_waiters, &wait);
	
	while(len > 0){
		unsigned long flags;
		spin_lock_irqsave(&state->lock, &flags);

		bool data_ready = false;
		if (t->c_lflag & ICANON) {
			if (state->lines > 0 || state->eof_pending) {
				data_ready = true;
			}
		} else {
			if (state->count > 0) {
				data_ready = true;
			}
		}

		if (!data_ready) {
			spin_unlock_irqrestore(&state->lock, &flags);
			if(bytes_read > 0){
				break;
			}
			sleep_current();
			continue;
		}

		if ((t->c_lflag & ICANON) && state->count == 0 && state->eof_pending) {
			state->eof_pending = false;
			if (state->lines > 0)
				state->lines--;
			spin_unlock_irqrestore(&state->lock, &flags);
			break;
		}

		int line_done = 0;
		while(state->count > 0 && len > 0){
			u8 ch = state->buffer[state->tail];
			state->tail = (state->tail + 1) % TTY_BUFFER_LDISC_SIZE;
			state->count--;

			buffer[bytes_read++] = ch;
			len--;

			if ((t->c_lflag & ICANON) &&
			    (ch == '\n' || ch == '\r' ||
			     (t->c_cc[VEOL] && ch == t->c_cc[VEOL]) ||
			     (t->c_cc[VEOL2] && ch == t->c_cc[VEOL2]))) {
				line_done = 1;
				if (state->lines > 0)
					state->lines--;
				break;
			}
		}

		if ((t->c_lflag & ICANON) && state->count == 0 && state->lines > 0 && !line_done) {
            state->lines--;
        }

		spin_unlock_irqrestore(&state->lock, &flags);

		if (t->c_lflag & ICANON) {
			if (line_done)
				break;
		} else {
			if (bytes_read > 0)
				break;
		}
	}

	wait_queue_remove(&tty->read_waiters, &wait);
	return bytes_read;
}

static int tty_ldisc_write(struct tty_struct* tty, const u8 *buffer, size_t len){
	if(!tty || !buffer || len == 0){
		return 0;
	}

	if(!tty->ops || !tty->ops->write){
		return -ENODEV;
	}

	const struct termios *t = &tty->termios;

	/* Output processing (OPOST & ONLCR) */
	if ((t->c_oflag & OPOST) && (t->c_oflag & ONLCR)) {
		size_t written = 0;
		for (size_t i = 0; i < len; i++) {
			if (buffer[i] == '\n') {
				u8 crlf[2] = {'\r', '\n'};
				tty->ops->write(tty, crlf, 2);
			} else {
				tty->ops->write(tty, &buffer[i], 1);
			}
			written++;
		}
		return written;
	}

	return tty->ops->write(tty, buffer, len);
}

static bool tty_ldisc_pop_char(struct tty_ldisc_data *state, u8 *out_ch) {
	if (state->count == 0)
		return false;

	state->head = (state->head + TTY_BUFFER_LDISC_SIZE - 1) % TTY_BUFFER_LDISC_SIZE;
	state->count--;
	if (out_ch)
		*out_ch = state->buffer[state->head];
	state->buffer[state->head] = '\0';
	return true;
}

static void tty_ldisc_erase(struct tty_struct *tty, struct tty_ldisc_data *state){
	const struct termios *t = &tty->termios;

	if (tty_ldisc_pop_char(state, NULL)) {
		if ((t->c_lflag & (ECHO | ECHOE)) == (ECHO | ECHOE) && tty->ops && tty->ops->write) {
			tty->ops->write(tty, (const u8*)"\b \b", 3);
		}
	}
}

static void tty_ldisc_kill_line(struct tty_struct *tty, struct tty_ldisc_data *state){
	const struct termios *t = &tty->termios;

	if (state->count > 0) {
		while (tty_ldisc_pop_char(state, NULL));
		if ((t->c_lflag & (ECHO | ECHOK)) == (ECHO | ECHOK) && tty->ops && tty->ops->write) {
			tty->ops->write(tty, (const u8*)"\n", 1);
		}
	}
}

static void tty_ldisc_werase(struct tty_struct *tty, struct tty_ldisc_data *state){
	const struct termios *t = &tty->termios;
	u8 ch;

	/* Phase 1: Erase trailing spaces */
	while (state->count > 0) {
		size_t prev_idx = (state->head + TTY_BUFFER_LDISC_SIZE - 1) % TTY_BUFFER_LDISC_SIZE;
		if (state->buffer[prev_idx] != ' ')
			break;

		tty_ldisc_pop_char(state, &ch);
		if ((t->c_lflag & (ECHO | ECHOE)) == (ECHO | ECHOE) && tty->ops && tty->ops->write) {
			tty->ops->write(tty, (const u8*)"\b \b", 3);
		}
	}

	/* Phase 2: Erase word characters until next space */
	while (state->count > 0) {
		size_t prev_idx = (state->head + TTY_BUFFER_LDISC_SIZE - 1) % TTY_BUFFER_LDISC_SIZE;
		if (state->buffer[prev_idx] == ' ')
			break;

		tty_ldisc_pop_char(state, &ch);
		if ((t->c_lflag & (ECHO | ECHOE)) == (ECHO | ECHOE) && tty->ops && tty->ops->write) {
			tty->ops->write(tty, (const u8*)"\b \b", 3);
		}
	}
}

static int tty_ldisc_should_wake_readers(struct tty_struct *tty, struct tty_ldisc_data *state, u8 ch){
	const struct termios *t = &tty->termios;

	if (!(t->c_lflag & ICANON))
		return 1;

	if (state->lines > 0 || state->eof_pending)
		return 1;

	return 0;
}

static int tty_ldisc_input_process(struct tty_struct *tty, struct tty_ldisc_data *state, u8* ch)
{
	const struct termios *t = &tty->termios;

	if (t->c_iflag & ISTRIP)
		*ch &= 0x7f;

	if ((t->c_iflag & IGNCR) && *ch == '\r')
		return 1;

	if ((t->c_iflag & ICRNL) && *ch == '\r')
		*ch = '\n';
	else if ((t->c_iflag & INLCR) && *ch == '\n')
		*ch = '\r';

	return 0;
}

static int tty_ldisc_canonical_process(
    struct tty_struct *tty,
    struct tty_ldisc_data *state,
    u8 ch)
{
	const struct termios *t = &tty->termios;

	if (!(t->c_lflag & ICANON))
		return 0;

	if (t->c_cc[VERASE] && ch == t->c_cc[VERASE]) {
		tty_ldisc_erase(tty, state);
		return 1;
	}

	if (t->c_cc[VKILL] && ch == t->c_cc[VKILL]) {
		tty_ldisc_kill_line(tty, state);
		return 1;
	}

	if (t->c_cc[VWERASE] && ch == t->c_cc[VWERASE]) {
		tty_ldisc_werase(tty, state);
		return 1;
	}

	if (t->c_cc[VEOF] && ch == t->c_cc[VEOF]) {
		if (state->count == 0) {
			state->eof_pending = true;
		} else {
			state->lines++;
		}

		return 1;
	}

	return 0;
}

static void tty_ldisc_echo(struct tty_struct *tty,
                           struct tty_ldisc_data *state,
                           unsigned char ch)
{
	const struct termios *t = &tty->termios;

	if (!tty->ops || !tty->ops->write)
		return;

	if ((t->c_lflag & ECHO) || ((t->c_lflag & ECHONL) && ch == '\n')) {
		if ((t->c_oflag & OPOST) && (t->c_oflag & ONLCR) && ch == '\n') {
			u8 crlf[2] = {'\r', '\n'};
			tty->ops->write(tty, crlf, 2);
		} else {
			tty->ops->write(tty, &ch, 1);
		}
	}
}

static int tty_ldisc_store_char(struct tty_struct* tty, struct tty_ldisc_data* state, unsigned char ch){
	if(state->count == TTY_BUFFER_LDISC_SIZE){
		return -EAGAIN;
	}

	state->buffer[state->head] = ch;
	state->head = (state->head + 1) % TTY_BUFFER_LDISC_SIZE;
	state->count++;

	return 0;
}

int tty_ldisc_receive_buf(struct tty_struct* tty, const u8* data, size_t len){
	if(!tty || !data || len == 0){
		return 0;
	}

	if(!tty->ldisc || !tty->ldisc_data){
		return -ENODEV;
	}

	unsigned long flags;
	bool wake_up = false;
	int ret = 0;

	struct tty_ldisc_data* state = tty->ldisc_data;

	spin_lock_irqsave(&state->lock, &flags);

	for(size_t i = 0; i < len; i++){
		unsigned char ch = data[i];

		/*
		* 1. Input processing
		*/
		ret = tty_ldisc_input_process(tty, state, &ch);

		if (ret < 0){
			spin_unlock_irqrestore(&state->lock, &flags);
			return ret;
		}

		if (ret > 0)
			continue;

		/*
		* 2. Canonical editing
		*/
		ret = tty_ldisc_canonical_process(tty, state, ch);

		if (ret < 0){
			spin_unlock_irqrestore(&state->lock, &flags);
			return ret;
		}

		if (ret > 0) {
			goto check_wake_up;
		}

		/*
		* 3. Store the character for userspace.
		*/
		ret = tty_ldisc_store_char(tty, state, ch);

		if (ret < 0){
			spin_unlock_irqrestore(&state->lock, &flags);
			return ret;
		}

		const struct termios *t = &tty->termios;
		if ((t->c_lflag & ICANON) &&
		    (ch == '\n' || ch == '\r' ||
		     (t->c_cc[VEOL] && ch == t->c_cc[VEOL]) ||
		     (t->c_cc[VEOL2] && ch == t->c_cc[VEOL2]))) {
			state->lines++;
		}

		/*
		* 4. Echo
		*/
		tty_ldisc_echo(tty, state, ch);

		/*
		* 5. Wake readers if this character completes
		*    something they can consume.
		*/

check_wake_up:
		if (tty_ldisc_should_wake_readers(tty, state, ch)) {
			wake_up = true;
		}
	}

	spin_unlock_irqrestore(&state->lock, &flags);

	if(wake_up){
		wake_up(&tty->read_waiters);
	}

	return 0;
}

const struct tty_ldisc_ops tty_ldisc_ops = {
	.open = tty_ldisc_open,
	.close = tty_ldisc_close,
	.read = tty_ldisc_read,
	.write = tty_ldisc_write,

	.receive_buf = tty_ldisc_receive_buf,
};

void tty_receive_char(struct tty_struct* tty, char ch){
	if(!tty || !tty->ldisc || !tty->ldisc->ops || !tty->ldisc->ops->receive_buf){
		return;
	}

	tty->ldisc->ops->receive_buf(tty, (u8*)&ch, 1);
}
