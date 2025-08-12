#ifndef _FILE_H
#define _FILE_H

#include <stdint.h>

#define S_IFMT  0170000
#define S_IFLNK 0120000
#define S_IFREG 0100000
#define S_IFBLK 0060000
#define S_IFDIR 0040000

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct stat {
    uint32_t mode;
    uint32_t size;
    uint32_t uid;
    uint8_t attr;
    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;
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
