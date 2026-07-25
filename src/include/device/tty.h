#ifndef _TTY_H
#define _TTY_H

#include <sync/spinlock.h>
#include <lib/list.h>
#include <stddef.h>

struct tty_struct;
struct tty_driver;
struct device;
struct file;

struct tty_ops {
	int (*install)(struct tty_driver*, struct tty_struct *new_tty);
	int (*open)(struct tty_struct*, struct file *file);
	int (*write)(struct tty_struct*, const char *buffer, size_t count);
	int (*ioctl)(struct tty_struct*, unsigned int cmd, unsigned long arg);
};

struct tty_ldisc_ops {
	int (*open)(struct tty_struct*);
	int (*close)(struct tty_struct*);
	int (*receive_buf)(struct tty_struct*, const char *data, size_t len);
};

struct tty_ldisc {
	const struct tty_ldisc_ops *ops;
	void* data;
};

struct tty_line_discipline_state {
	struct tty_ldisc ldisc;
	char buffer[256];
	size_t len;
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

	void* private;

	spinlock_t lock;
};

struct tty_driver* tty_alloc_drive();
int tty_register_driver(struct tty_driver* driver);

struct tty_struct* tty_ensure_created(struct tty_driver* driver, const int index);

void tty_receive_char(struct tty_struct* tty, char ch);

#endif
