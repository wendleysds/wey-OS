#ifndef _VIRTUAL_FILE_SYSTEM_H
#define _VIRTUAL_FILE_SYSTEM_H

#include <wey/blkdev.h>
#include <wey/stat.h>
#include <mm/kheap.h>
#include <lib/list.h>
#include <stdint.h>

#define FMODE_READ   0x1
#define FMODE_WRITE  0x2
#define FMODE_LSEEK  0x4
#define FMODE_EXEC   0x8

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct inode {
	uint32_t ino;

	uint16_t mode;
	uint32_t size;
	void *private_data;

	dev_t i_rdev;

	struct super_block* i_sb;
	const struct inode_operations *i_op;
	const struct file_operations *i_fop;

	struct list_head i_sb_list;
};

struct file {
    struct inode *inode;
    uint32_t pos;
    uint32_t flags;
    void *private_data;

    const struct file_operations *f_op;
};

struct stat {
    uint32_t mode;
    uint32_t size;
    uint32_t uid;
    uint8_t attr;
    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;
};

struct file_operations {
    int (*read)(struct file *file, void *buffer, uint32_t count);
    int (*write)(struct file *file, const void *buffer, uint32_t count);
    int (*lseek)(struct file *file, int offset, int whence);
	int (*open) (struct inode *ino, struct file *file);
    int (*close)(struct file *file);
	int (*release) (struct inode *ino, struct file *file);
};

struct inode_operations {
    struct inode* (*lookup)(struct inode *dir, const char *name);
    int (*create)(struct inode *dir, const char *name, uint16_t mode);
    int (*unlink)(struct inode *dir, const char *name);
    int (*mkdir)(struct inode *dir, const char *name);
    int (*rmdir)(struct inode *dir, const char *name);
    int (*getattr)(struct inode *dir, const char *name, struct stat* restrict statbuf);
    int (*setarrt)(struct inode *dir, const char *name, uint16_t attr);
};

struct file_system_type{
	const char *name;

	struct inode* (*mount)(struct file_system_type*, int flags, const char* dev_name, void* data);
	int (*unmount)(struct super_block*);
};

struct super_block {
	struct list_head s_sbs;
	
	struct inode *root_inode;
	void *private_data;
	unsigned long flags;

	struct file_system_type* fs_type;

	const struct super_operations* s_op;

	struct blkdev* bdev;
	struct file* bdev_file;

	struct list_head s_inodes;
};

struct super_operations{
	struct inode* (*alloc_inode)(struct super_block*);
	void (*destroy_inode)(struct inode*);
	void (*free_inode)(struct inode*);

	int (*statfs)(struct inode*, struct stat*);
};

struct mount {
	char *mountpoint;
	struct inode* mnt_root;
	struct super_block *mnt_sb;
	struct mount *next;
};

extern struct mount* mnt_root;

int vfs_mount(const char* source, const char *mountpoint, const char *filesystemtype, void* data);
int vfs_umount(const char *mountpoint);

struct inode* vfs_lookup(const char *restrict path);
struct file* vfs_open(const char *restrict path, uint32_t flags);

int vfs_create(const char *restrict path, uint16_t mode);
int vfs_unlink(const char *restrict path);
int vfs_mkdir(const char *restrict path);
int vfs_rmdir(const char *restrict path);

int vfs_read(struct file *file, void *buffer, uint32_t size);
int vfs_lseek(struct file *file, int offset, int whence);
int vfs_write(struct file *file, const void *buffer, uint32_t size);
int vfs_close(struct file *file);
int vfs_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

int vfs_getattr(const char *restrict path, struct stat *restrict statbuf);

int vfs_register_filesystem(struct file_system_type* fs);
int vfs_unregister_filesystem(struct file_system_type* fs);

struct super_block* alloc_super();

struct inode* inode_alloc(struct super_block *sb);
struct inode* inode_new(struct super_block *sb);

void inode_destroy(struct inode*);

int kernel_exec(const char* pathname, const char* argv[], const char* envp[]);

#endif
