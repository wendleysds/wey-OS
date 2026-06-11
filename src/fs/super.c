#include "lib/list.h"
#include "sync/spinlock.h"
#include <def/config.h>
#include <def/err.h>
#include <fs/vfs.h>
#include <kernel/init.h>
#include <lib/string.h>

static LIST_HEAD(file_systems);
static spinlock_t file_system_lock;

struct mount *root_mount = NULL;
static spinlock_t mount_lock;

void vfs_register_filesystem(struct file_system_type *fs) {
	spin_lock(&file_system_lock);
	INIT_LIST_HEAD(&fs->list);
	list_add(&fs->list, &file_systems);
	spin_unlock(&file_system_lock);
}

void vfs_unregister_filesystem(struct file_system_type *fs) {
	spin_lock(&file_system_lock);
	list_remove(&fs->list);
	spin_unlock(&file_system_lock);
}

static const struct file_system_type *find_fs_by_name(const char *name) {
	struct file_system_type *tmp, *fs = NULL;
	spin_lock(&file_system_lock);

	list_for_each_entry(tmp, &file_systems, list) {
	if (strcmp(tmp->name, name) == 0) {
			fs = tmp;
			break;
		}
	}

	spin_unlock(&file_system_lock);
	return fs;
}

struct super_block *super_alloc() {
	struct super_block *sb = (struct super_block*)kzalloc(sizeof(struct super_block));
	if (sb) {
		INIT_LIST_HEAD(&sb->s_sbs);
		INIT_LIST_HEAD(&sb->s_inodes);
		spinlock_init(&sb->s_inode_lock);
	}
	return sb;
}

void super_destroy(struct super_block *sb) { 
	kfree(sb); 
}

static struct mount *get_parent_mount(const char *mountpoint) {
	struct mount *tmp, *mount = root_mount;
	struct qstr comp;

	const char *cursor = mountpoint;
	while (path_iterate(&cursor, &comp)) {
		list_for_each_entry(tmp, &mount->children, sibling) {
			if (strncmp(tmp->name, comp.name, comp.len) == 0) {
				mount = tmp;
				break;
			}
		}
	}

	return mount;
}

int vfs_mount(const char *source, const char *mountpoint, const char *fs_name, unsigned int flags, void *data) {
	const struct file_system_type *target_fs = find_fs_by_name(fs_name);
	struct inode *point = NULL;
	struct mount *mount = NULL;
	struct inode *root = NULL;

	int ret;

	if (!target_fs) {
		return NO_ENTRY;
	}

	if (root_mount) {
		point = vfs_walk(mountpoint);
		if (IS_ERR(point)) {
			return PTR_ERR(point);
		}

		spin_lock(&point->lock);
	}

	mount = kmalloc(sizeof(*mount));
	if (!mount) {
		ret = NO_MEMORY;
		goto out_point;
	}

	spin_lock(&mount_lock);

	root = target_fs->mount(target_fs, source, data);
	if (IS_ERR_OR_NULL(root)) {
		ret = root ? PTR_ERR(root) : FAILED;
		spin_unlock(&mount_lock);
		goto out_mount;
	}

	memset(mount, 0x0, sizeof(*mount));
	INIT_LIST_HEAD(&mount->children);
	INIT_LIST_HEAD(&mount->sibling);

	mount->mnt_sb = root->i_sb;
	mount->mnt_root = root;

	if (root_mount) {
		inode_get(point);
		point->mounted_here = mount;
	} else {
		root_mount = mount;
		spin_unlock(&mount_lock);
		return SUCCESS;
	}

	struct mount *p = get_parent_mount(mountpoint);
	list_add(&mount->sibling, &p->children);
	mount->parent = p;

	spin_unlock(&mount_lock);
	spin_unlock(&point->lock);
	return SUCCESS;

out_mount:
	kfree(mount);
out_point:
	if (point) {
		inode_put(point);
		spin_unlock(&point->lock);
	}

	return ret;
}

int vfs_umount(const char *mountpoint) {
	struct inode *point = vfs_walk(mountpoint);
	struct mount *mnt;
	struct super_block *sb;
	struct inode *ino, *tmp;

	int ret = SUCCESS;

	if (IS_ERR(point)) {
		return PTR_ERR(point);
	}

	spin_lock(&point->lock);
	spin_lock(&mount_lock);

	mnt = point->mounted_here;
	if (!mnt) {
		ret = INVALID_ARG;
		goto out_unlock;
	}

	if (!list_empty(&mnt->children)) {
		ret = BUSY;
		goto out_unlock;
	}

	sb = mnt->mnt_sb;
	list_for_each_entry(ino, &sb->s_inodes, i_sb_list) {
		if (atomic_read(&ino->refcount) > 1) {
			ret = BUSY;
			goto out_unlock;
		}
	}

	if (sb->fs_type->unmount) {
		sb->fs_type->unmount(sb);
	}

	list_for_each_entry_safe(ino, tmp, &sb->s_inodes, i_sb_list) {
		inode_destroy(ino);
	}

	if (mnt->parent) {
		list_remove(&mnt->sibling);
	}

	if (mnt == root_mount) {
		root_mount = NULL;
	}

	kfree(mnt);
	super_destroy(sb);
	point->mounted_here = NULL;

out_unlock:
	spin_unlock(&mount_lock);
	spin_unlock(&point->lock);
	inode_put(point);
	return ret;
}

static __init int vfs_super_init() {
	root_mount = NULL;
	INIT_LIST_HEAD(&file_systems);
	spinlock_init(&file_system_lock);
	spinlock_init(&mount_lock);
	return SUCCESS;
}

core_initcall(vfs_super_init);