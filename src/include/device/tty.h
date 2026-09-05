#ifndef _TTY_H
#define _TTY_H

#include <kernel/wait.h>
#include <sync/spinlock.h>
#include <lib/list.h>
#include <stddef.h>

#include <uapi/kernel/termios.h>

struct tty_struct;
struct tty_driver;
struct device;
struct file;

struct tty_ops {
	int (*install)(struct tty_driver*, struct tty_struct *new_tty);
	int (*open)(struct tty_struct*, struct file *file);
	int (*write)(struct tty_struct*, const u8 *buffer, size_t count);
	int (*ioctl)(struct tty_struct*, unsigned int cmd, unsigned long arg);
};

struct tty_ldisc_ops {
	int (*open)(struct tty_struct*);
	int (*close)(struct tty_struct*);

	int (*read)(struct tty_struct*, u8 *buffer, size_t len);
	int (*write)(struct tty_struct*, const u8 *buffer, size_t len);

	int (*receive_buf)(struct tty_struct*, const u8 *data, size_t len);
};

struct tty_ldisc {
	const struct tty_ldisc_ops *ops;
	struct tty_struct* tty;
};

struct tty_buffer {
	struct tty_buffer* next;

	unsigned int read_pos;
	unsigned int write_pos;

	unsigned int size;
	u8* data;
};

struct tty_bufhead {
	struct tty_buffer *head;
	struct tty_buffer *tail; // Active buffer

	spinlock_t lock;
};

struct tty_driver {
	const char *name;
	unsigned int major;
	unsigned int minor_start;
	unsigned int num;

	struct tty_struct** ttys;
	struct device** devs;

	const struct tty_ops *ops;
	struct list_head list;
};

struct tty_struct {
	int index;
	struct tty_driver *driver;
	const struct tty_ops *ops;

	struct tty_ldisc *ldisc;
	void* ldisc_data;

	struct termios termios;

	void* private;

	struct tty_bufhead buffer;

	struct wait_queue_head read_waiters;
	spinlock_t lock;
};

struct tty_driver* tty_alloc_driver();
int tty_register_driver(struct tty_driver* driver);

struct tty_struct* tty_ensure_created(struct tty_driver* driver, const int index);

int tty_ldisc_receive_buf(struct tty_struct* tty, const u8* data, size_t len);

int tty_bufhead_init(struct tty_bufhead* bufhead);
int tty_receive_buf(struct tty_struct* tty, const u8* data, size_t len);

#endif
