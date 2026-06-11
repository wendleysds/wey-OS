#ifndef _VIRTUAL_FILE_SYSTEM_H
#define _VIRTUAL_FILE_SYSTEM_H

#include <sync/spinlock.h>
#include <lib/list.h>
#include <mm/kheap.h>

#define FMODE_READ   0x1
#define FMODE_WRITE  0x2
#define FMODE_LSEEK  0x4
#define FMODE_EXEC   0x8

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define ATTR_MODE  (1 << 0)
#define ATTR_SIZE  (1 << 1)
#define ATTR_DEV   (1 << 2)
#define ATTR_ATIME (1 << 3)
#define ATTR_MTIME (1 << 4)
#define ATTR_CTIME (1 << 5)

struct mount;

struct inode {
	uint64_t ino;

	umode_t mode;

	dev_t dev;

	size_t size;

	atomic_t refcount;
	spinlock_t lock;

	void *private_data;
	struct mount *mounted_here;

	struct super_block* i_sb;
	const struct inode_operations *i_op;
	const struct file_operations *i_fop;

	timespan atime_sec;
	timespan mtime_sec;
	timespan ctime_sec;
	atomic_t version;

	struct list_head i_sb_list;
};

struct file {
	spinlock_t lock;
	atomic_t refcount;

	struct inode *inode;
	void *private_data;

	off_t pos;
	unsigned int flags;
	const struct file_operations *f_op;
};

struct stat {
	uint64_t ino;
	umode_t mode;

	dev_t rdev;
	dev_t dev;

	size_t size;

	timespan atime;
	timespan mtime;
	timespan ctime;
};

struct iattr {
	uint32_t valid;
	struct stat stat;
};

struct qstr {
	const char* name;
    size_t len;
};

struct file_operations {
	int (*read)(struct file *file, void *buffer, uint32_t count);
	int (*write)(struct file *file, const void *buffer, uint32_t count);
	off_t (*lseek)(struct file *file, off_t offset, int whence);
	int (*open) (struct inode *ino, struct file *file);
	int (*close)(struct file *file);
	int (*release) (struct inode *ino, struct file *file);
};

struct inode_operations {
	struct inode* (*lookup)(struct inode *dir, struct qstr *name);
	int (*create)(struct inode *dir, struct qstr *name, uint16_t mode);
	int (*unlink)(struct inode *dir, struct qstr *name);
	int (*mkdir)(struct inode *dir, struct qstr *name);
	int (*rmdir)(struct inode *dir, struct qstr *name);
	int (*getattr)(struct inode *ino, struct stat* restrict statbuf);
	int (*setarrt)(struct inode *ino, struct iattr* attr);
	int (*mknod)(struct inode *dir, struct qstr *name, uint16_t mode, dev_t dev);
};

struct file_system_type{
	const char *name;

	struct inode* (*mount)(const struct file_system_type*, const char* dev_name, void* data);
	void (*unmount)(struct super_block*);

	struct list_head list;
};

struct super_block {
	struct list_head s_sbs;
	
	struct inode *root_inode;
	void *private_data;
	unsigned long flags;

	const struct file_system_type* fs_type;
	const struct super_operations* s_op;

	struct blkdev* bdev;
	struct file* bdev_file;

	atomic_t refcount;
	spinlock_t s_inode_lock;
	struct list_head s_inodes;
};

struct super_operations{
	struct inode* (*alloc_inode)(struct super_block*);
	void (*destroy_inode)(struct inode*);
	void (*free_inode)(struct inode*);

	int (*statfs)(struct inode*, struct stat*);
};

struct mount {
	char *name;
	struct inode *mnt_root;
	struct inode *mnt_mountpoint;
	struct super_block *mnt_sb;

	struct mount *parent;
	struct list_head children;
	struct list_head sibling;
};

struct path {
	struct mount *mount;
	struct inode *dentry;
};

int vfs_mount(const char* source, const char *mountpoint, const char *filesystemtype, unsigned int flags, void* data);
int vfs_umount(const char *mountpoint);

int vfs_lookup_path(struct inode *parent, struct qstr *name, struct path *res);
int vfs_walk_path(const char *path, struct path *res);

struct inode* vfs_lookup(struct inode *parent, struct qstr *name);
struct inode* vfs_walk_parent(const char *path, struct qstr *last);
struct inode* vfs_walk(const char *path);

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

void vfs_register_filesystem(struct file_system_type* fs);
void vfs_unregister_filesystem(struct file_system_type* fs);

int path_iterate(const char** cursor, struct qstr* comp);

struct super_block* super_alloc();
void destroy_super(struct super_block* sb);

struct inode* inode_alloc(struct super_block *sb);
struct inode* inode_new(struct super_block *sb);
void inode_destroy(struct inode*);

static inline void inode_get(struct inode* ino){
	atomic_inc(&ino->refcount);
}

static inline void inode_put(struct inode* ino){
	if(atomic_dec_and_test(&ino->refcount)){
		inode_destroy(ino);
	}
}

static inline void super_get(struct super_block* sb){
	atomic_inc(&sb->refcount);
}

static inline void super_put(struct super_block* sb){
	if(atomic_dec_and_test(&sb->refcount)){
		destroy_super(sb);
	}
}

static inline void file_get(struct file* file){
	atomic_inc(&file->refcount);
}

static inline void file_put(struct file* file){
	if(atomic_dec_and_test(&file->refcount)){
		if(file->f_op && file->f_op->release){
			file->f_op->release(file->inode, file);
		}

		if (file->inode){
			inode_put(file->inode);
		}

		kfree(file);
	}
}

int kernel_exec(const char* pathname, const char* argv[], const char* envp[]);

#endif
