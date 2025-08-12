#ifndef _VIRTUAL_FILE_SYSTEM_H
#define _VIRTUAL_FILE_SYSTEM_H

#include <fs/file.h>
#include <memory/kheap.h>
#include <device.h>
#include <stdint.h>

struct inode_operations {
    struct inode* (*lookup)(struct inode *dir, const char *name);
    int (*create)(struct inode *dir, const char *name, uint16_t mode);
    int (*unlink)(struct inode *dir, const char *name);
    int (*mkdir)(struct inode *dir, const char *name);
    int (*rmdir)(struct inode *dir, const char *name);
    int (*getattr)(struct inode *dir, const char *name, struct stat* restrict statbuf);
    int (*setarrt)(struct inode *dir, const char *name, uint16_t attr);
};

struct superblock {
    struct inode *root_inode;
    void *private_data;
};

struct filesystem {
    const char *name;
    int (*mount)(struct superblock* sb, struct device* device);
    int (*unmount)(struct superblock* sb);
    struct inode* (*get_root)(struct superblock* sb);
};

struct mount {
    char *mountpoint;
    struct filesystem *fs;
    struct superblock *sb;

    struct mount *next;
};

extern struct inode* vfs_root_node;
extern struct mount* mnt_root;

int vfs_mount(struct device *device, const char *mountpoint, const char *filesystemtype);
int vfs_umount(const char *mountpoint);

struct inode* vfs_lookup(const char *restrict path);
struct file* vfs_open(const char *restrict path, uint32_t flags);

int vfs_create(const char *restrict path, uint16_t mode);
int vfs_unlink(const char *restrict path);
int vfs_mkdir(const char *restrict path);
int vfs_rmdir(const char *restrict path);
int vfs_getattr(const char *restrict path, struct stat *restrict statbuf);
int vfs_setattr(const char *restrict path, uint16_t attr);

int vfs_register_filesystem(struct filesystem* fs);
int vfs_unregister_filesystem(struct filesystem* fs);

static inline void inode_dispose(struct inode *ino) {
    if (ino->private_data)
        kfree(ino->private_data);

    kfree(ino);
}

#endif
