#include <def/config.h>
#include <def/errno.h>
#include <fs/vfs.h>
#include <fs/dcache.h>
#include <fs/stat.h>
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

static int do_umount(struct mount* mount){
	struct super_block *sb;
	struct inode *ino, *tmp;

	int ret = SUCCESS;

	if (!list_empty(&mount->children)) {
		ret = -EBUSY;
		goto out_unlock;
	}

	if (mount->mnt_root) {
        dentry_put(mount->mnt_root);
        mount->mnt_root = NULL;
    }

	sb = mount->mnt_sb;
	list_for_each_entry(ino, &sb->s_inodes, i_sb_list) {
		if (atomic_read(&ino->refcount) > 1) {
			ret = -EBUSY;
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

	kfree(mount);
	super_destroy(sb);

out_unlock:
	return ret;
}

static struct dentry *d_make_root(struct inode *root_inode) {
	if (!root_inode) return NULL;
	struct qstr root_name = { .name = "/", .len = 1 };
	return dcache_add(NULL, &root_name, root_inode);
}

int vfs_mount(const char *source, const char *mountpoint, const char *fs_name, unsigned int flags, void *data) {
	const struct file_system_type *target_fs = find_fs_by_name(fs_name);
	struct path target_point;
	struct mount *mount = NULL;
	struct inode *root_inode = NULL;
	struct dentry *root_dentry = NULL;
	int ret;

	memset(&target_point, 0, sizeof(struct path));

	if (!target_fs) {
		return -ENOENT;
	}

	if (root_mount) {
		ret = vfs_walk_path(mountpoint, &target_point);
		if (IS_ERR_VALUE(ret)) {
			return ret;
		}

		if (!S_ISDIR(target_point.dentry->inode->mode)) {
			ret = -ENOTDIR;
			goto out_point;
		}

		spin_lock(&target_point.dentry->lock);
		if (target_point.dentry->mounted_here) {
			spin_unlock(&target_point.dentry->lock);
			ret = -EBUSY;
			goto out_point;
		}
		spin_unlock(&target_point.dentry->lock);
	}

	mount = kzalloc(sizeof(*mount));
	if (!mount) {
		ret = -ENOMEM;
		goto out_point;
	}

	INIT_LIST_HEAD(&mount->children);
	INIT_LIST_HEAD(&mount->sibling);

	root_inode = target_fs->mount(target_fs, source, data);
	if (IS_ERR_OR_NULL(root_inode)) {
		ret = root_inode ? PTR_ERR(root_inode) : -EAGAIN;
		goto out_mount;
	}

	root_dentry = d_make_root(root_inode);
	if (!root_dentry) {
		ret = -ENOMEM;
		inode_destroy(root_inode);
		goto out_mount;
	}

	inode_put(root_inode);
	mount->mnt_sb = root_inode->i_sb;
	mount->mnt_root = root_dentry;

	spin_lock(&mount_lock);

	if (!root_mount) {
		mount->mnt_mountpoint = NULL;
		mount->parent = mount;
		root_mount = mount;
		spin_unlock(&mount_lock);
		return SUCCESS;
	}

	// inherits the reference obtained in vfs_walk_path
	mount->mnt_mountpoint = target_point.dentry;
	mount->parent = target_point.mount;
	list_add(&mount->sibling, &target_point.mount->children);

	spin_lock(&target_point.dentry->lock);
	target_point.dentry->mounted_here = mount;
	spin_unlock(&target_point.dentry->lock);

	spin_unlock(&mount_lock);
	return SUCCESS;

out_mount:
	kfree(mount);
out_point:
	if (target_point.dentry) {
		dentry_put(target_point.dentry);
	}
	return ret;
}

int vfs_umount(const char *mountpoint) {
	struct path target_path;
	int err = vfs_walk_path(mountpoint, &target_path);
	if (err != SUCCESS) return err;

	struct mount *mnt = target_path.mount;
	dentry_put(target_path.dentry);

	if (mnt == root_mount) {
		return -EBUSY;
	}

	spin_lock(&mount_lock);
	struct dentry *point = mnt->mnt_mountpoint;
	if (point) {
		spin_lock(&point->lock);
		point->mounted_here = NULL;
	}
	
	int ret = do_umount(mnt);

	if (unlikely(ret != SUCCESS) && point) {
		point->mounted_here = mnt;
		spin_unlock(&point->lock);
	} else if (point) {
		dentry_put(point);
		spin_unlock(&point->lock);
	}

	spin_unlock(&mount_lock);
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
