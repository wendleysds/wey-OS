#ifndef _FILE_H
#define _FILE_H

#include <stdint.h>

struct stat {
	uint32_t fileSize;
	uint8_t attr;
	uint16_t creDate;
	uint16_t modDate;
};

struct inode {
    uint32_t ino;
    uint32_t mode;
    uint32_t size;
    void *private_data;

    struct inode_operations *i_op;
    struct file_operations *i_fop;
};

struct file {
    struct inode *inode;
    uint32_t pos;
    uint32_t flags;
    void *private_data;

    struct file_operations *f_op;
};

struct file_operations {
    int (*read)(struct file *file, void *buffer, uint32_t count);
    int (*write)(struct file *file, const void *buffer, uint32_t count);
    int (*lseek)(struct file *file, int offset, int whence);
    int (*close)(struct file *file);
};

#endif
