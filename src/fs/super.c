#include "lib/list.h"
#include "sync/atomic.h"
#include "sync/spinlock.h"
#include <def/config.h>
#include <def/err.h>
#include <fs/vfs.h>
#include <kernel/init.h>
#include <lib/string.h>
#include <kernel/printk.h>

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

static int mount_name_match(const struct mount *mount, const struct qstr *name) {
	return mount->name && strlen(mount->name) == name->len &&
		strncmp(mount->name, name->name, name->len) == 0;
}

static char *dup_mount_name(const char *mountpoint) {
	struct qstr comp, last = {0};
	const char *cursor = mountpoint;

	while (path_iterate(&cursor, &comp)) {
		last = comp;
	}

	if (!last.name) {
		return strdup("/");
	}

	char *name = kmalloc(last.len + 1);
	if (!name) {
		return NULL;
	}

	memcpy(name, last.name, last.len);
	name[last.len] = '\0';
	return name;
}

static struct mount *find_child_mount(struct mount *parent, const struct qstr *name) {
	struct mount *child;

	list_for_each_entry(child, &parent->children, sibling) {
		if (mount_name_match(child, name)) {
			return child;
		}
	}

	return NULL;
}

static struct mount *find_mount(const char *mountpoint) {
	struct mount *mount = root_mount;
	struct qstr comp;
	const char *cursor = mountpoint;

	if (!mount) {
		return NULL;
	}

	while (path_iterate(&cursor, &comp)) {
		mount = find_child_mount(mount, &comp);
		if (!mount) {
			return NULL;
		}
	}

	return mount;
}

static struct mount *get_parent_mount(const char *mountpoint) {
	struct mount *tmp, *mount = root_mount;
	struct qstr comp;

	const char *cursor = mountpoint;
	while (path_iterate(&cursor, &comp)) {
		const char *next = cursor;
		struct qstr next_comp;

		if (!path_iterate(&next, &next_comp)) {
			break;
		}

		tmp = find_child_mount(mount, &comp);
		if (tmp) {
			mount = tmp;
		}
	}

	return mount;
}

static int do_umount(struct mount* mount){
	struct super_block *sb;
	struct inode *ino, *tmp;

	int ret = SUCCESS;

	if (!list_empty(&mount->children)) {
		ret = BUSY;
		goto out_unlock;
	}

	sb = mount->mnt_sb;
	list_for_each_entry(ino, &sb->s_inodes, i_sb_list) {
		if (atomic_read(&ino->refcount) > 1) {
			ret = BUSY;
			goto out_unlock;
		}
	}

	list_for_each_entry_safe(ino, tmp, &sb->s_inodes, i_sb_list) {
		inode_destroy(ino);
	}

	if (sb->fs_type->unmount) {
		sb->fs_type->unmount(sb);
	}

	if (mount->parent) {
		list_remove(&mount->sibling);
	}

	if (mount == root_mount) {
		root_mount = NULL;
	}

	if (mount->name) {
		kfree(mount->name);
	}

	kfree(mount);
	super_destroy(sb);

out_unlock:
	return ret;
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
	}

	mount = kmalloc(sizeof(*mount));
	if (!mount) {
		ret = NO_MEMORY;
		goto out_point;
	}

	memset(mount, 0x0, sizeof(*mount));
	INIT_LIST_HEAD(&mount->children);
	INIT_LIST_HEAD(&mount->sibling);

	mount->name = dup_mount_name(mountpoint);
	if (!mount->name) {
		ret = NO_MEMORY;
		goto out_mount;
	}

	spin_lock(&mount_lock);

	root = target_fs->mount(target_fs, source, data);
	if (IS_ERR_OR_NULL(root)) {
		ret = root ? PTR_ERR(root) : FAILED;
		spin_unlock(&mount_lock);
		goto out_mount;
	}

	mount->mnt_sb = root->i_sb;
	mount->mnt_root = root;
	mount->mnt_mountpoint = point;

	if (root_mount) {
		spin_lock(&point->lock);
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
	if (mount->name) {
		kfree(mount->name);
	}
	kfree(mount);
out_point:
	if (point) {
		inode_put(point);
	}

	return ret;
}

int vfs_umount(const char *mountpoint) {
	spin_lock(&mount_lock);

	int ret = SUCCESS;
	int release_mountpoint = 0;
	struct mount *mnt = find_mount(mountpoint);
	if(!mnt){
		ret = INVALID_ARG;
		goto out;
	}

	struct inode *point = mnt->mnt_mountpoint;
	if(point){
		spin_lock(&point->lock);
	}

	if(point && point->mounted_here != mnt){
		ret = INVALID_ARG;
		goto out_point;
	}

	if(point){
		point->mounted_here = NULL;
	}

	ret = do_umount(mnt);

	if(unlikely(ret != SUCCESS) && point){
		point->mounted_here = mnt;
	} else if(point) {
		release_mountpoint = 1;
	}

out_point:
	if(point){
		spin_unlock(&point->lock);
	}
out:
	spin_unlock(&mount_lock);

	if(release_mountpoint){
		inode_put(point);
	}

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
