#ifndef _VIRTUAL_FILE_SYSTEM_H
#define _VIRTUAL_FILE_SYSTEM_H

#include <fs/fs.h>
#include <stdint.h>

struct inode_operations {
    struct inode* (*lookup)(struct inode *dir, const char *name);
    int (*create)(struct inode *dir, const char *name, uint16_t mode);
    int (*unlink)(struct inode *dir, const char *name);
    int (*mkdir)(struct inode *dir, const char *name);
    int (*rmdir)(struct inode *dir, const char *name);
    int (*stat)(struct inode *dir, const char *name, struct Stat* restrict statbuf);
    int (*setarrt)(struct inode *dir, const char *name, uint16_t attr);
};

struct superblock {
    struct inode *root_inode;
    void *private_data;
};

#endif