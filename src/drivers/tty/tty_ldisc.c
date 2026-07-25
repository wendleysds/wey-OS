#include <device/tty.h>
#include <device/terminal.h>
#include <kernel/printk.h>
#include <mm/kheap.h>
#include <def/errno.h>
#include <lib/string.h>

extern const struct tty_ldisc_ops tty_ldisc_ops;

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

static int tty_ldisc_receive_buf(struct tty_struct* tty, const char* data, size_t len){
	if(!tty || !data || len == 0){
		return 0;
	}

	if(!tty->ldisc || !tty->ldisc->data){
		return -ENODEV;
	}
	
	struct tty_line_discipline_state* state = tty->ldisc->data;
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

		if(ch == '\n' || ch == '\r'){
			if(state->len > 0){
				state->buffer[state->len] = '\0';
				printk("TTY line: %s\n", state->buffer);
			}

			state->len = 0;
			if(tty->ops && tty->ops->write){
				tty->ops->write(tty, "\n", 1);
			}
			continue;
		}

		if(ch >= 0x01 && ch <= 0x1A){
			char tmp[3] = { '^', 'A' + ch - 1, '\n' };

			state->len = 0;
			if(tty->ops && tty->ops->write){
				tty->ops->write(tty, tmp, 3);
			}
			continue;
		}

		if(state->len < sizeof(state->buffer) - 1){
			state->buffer[state->len++] = ch;
			if(tty->ops && tty->ops->write){
				tty->ops->write(tty, &ch, 1);
			}
		}
	}

	return 0;
}

const struct tty_ldisc_ops tty_ldisc_ops = {
	.open = tty_ldisc_open,
	.close = tty_ldisc_close,
	.receive_buf = tty_ldisc_receive_buf,
};

void tty_receive_char(struct tty_struct* tty, char ch){
	if(!tty || !tty->ldisc || !tty->ldisc->ops || !tty->ldisc->ops->receive_buf){
		return;
	}

	tty->ldisc->ops->receive_buf(tty, &ch, 1);
}
