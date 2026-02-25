#ifndef _CHRDEV_H
#define _CHRDEV_H

struct file_operations;

int chardev_register(unsigned int major, unsigned int base_minor, 
	unsigned int minor_total, const char *name, const struct file_operations *fops);

void chardev_unregister(unsigned int major, unsigned int base_minor,
	unsigned int minor_total, const char *name);

#endif