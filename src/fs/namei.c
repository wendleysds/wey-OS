#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/errno.h>
#include <fs/vfs.h>
#include <fs/stat.h>
#include <fs/dcache.h>

static DEFINE_SPINLOCK(rename_lock);

static inline bool is_dot(const struct qstr *n) {
	return
		(n->len == 1 && n->name[0] == '.') ||
		(n->len == 2 && n->name[0] == '.' && n->name[1] == '.');
}

int vfs_mknod(const char *restrict path, umode_t mode, dev_t dev){
	int res;
	struct qstr name;
	int trailing_slash = 0;
	struct dentry *parent_dentry = vfs_walk_parent(path, &name, &trailing_slash);
	if(IS_ERR_OR_NULL(parent_dentry)){
		return PTR_ERR(parent_dentry);
	}

	struct inode* parent = parent_dentry->inode;
	inode_get(parent);

	if(!parent->i_op || !parent->i_op->mknod){
		res = -ENOSYS;
		goto put_parent;
	}

	struct dentry dentry = {
		.parent = parent_dentry,
		.name = name,
	};

	res = parent->i_op->mknod(parent, &dentry, mode, dev);

put_parent:
	inode_put(parent);
	dentry_put(parent_dentry);
	return res;
}

int vfs_link(const char *restrict oldpath, const char *restrict newpath){
	struct dentry *old_d = vfs_walk(oldpath);
	if (IS_ERR(old_d)) return PTR_ERR(old_d);

	int res;
	if (S_ISDIR(old_d->inode->mode)) {
		res = -EPERM;
		goto out_old;
	}

	struct qstr name; int slash = 0;
	struct dentry *new_parent = vfs_walk_parent(newpath, &name, &slash);
	if (IS_ERR(new_parent)) {
		res = PTR_ERR(new_parent);
		goto out_old;
	}

	struct inode *dir = new_parent->inode;
	if (slash || is_dot(&name)) {
		res = -ENOENT;
	} else if (dir->i_sb != old_d->inode->i_sb) {
		res = -EXDEV;
	} else if (!dir->i_op || !dir->i_op->link) {
		res = -EPERM;
	} else {
		struct dentry new_stack = {
			.parent = new_parent,
			.name = name
		};

		res = dir->i_op->link(old_d, dir, &new_stack);
	}

	dentry_put(new_parent);
out_old:
	dentry_put(old_d);
	return res;
}

int vfs_unlink(const char *restrict path){
	struct qstr name; int trailing_slash = 0, res;
	struct dentry *parent_dentry = vfs_walk_parent(path, &name, &trailing_slash);
	if (IS_ERR(parent_dentry)) return PTR_ERR(parent_dentry);

	struct inode *parent = parent_dentry->inode;
	inode_get(parent);

	if (trailing_slash) {
		res = -ENOTDIR;
		goto put_parent;
	}

	if (is_dot(&name)) {
		res = -EISDIR;
		goto put_parent;
	}

	if (!parent->i_op || !parent->i_op->unlink) {
		res = -ENOSYS;
		goto put_parent;
	}

	struct path target;
	res = vfs_lookup_path(parent_dentry, &name, &target);
	if (res) goto put_parent;

	if (target.mount) {
		res = -EBUSY;
	} else if (S_ISDIR(target.dentry->inode->mode)) {
		res = -EISDIR;
	} else {
		res = parent->i_op->unlink(parent, target.dentry);
		if (!res) d_drop(target.dentry);
	}

	dentry_put(target.dentry);

put_parent:
	inode_put(parent);
	dentry_put(parent_dentry);
	return res;
}

int vfs_mkdir(const char *restrict path, umode_t mode){
	int res;
	struct qstr name;
	struct dentry *parent_dentry = vfs_walk_parent(path, &name, NULL);
	if(IS_ERR_OR_NULL(parent_dentry)){
		return PTR_ERR(parent_dentry);
	}

	struct inode* parent = parent_dentry->inode;
	inode_get(parent);

	if ((name.len == 1 && name.name[0] == '.') || 
		(name.len == 2 && name.name[0] == '.' && name.name[1] == '.')) {
		res = -EEXIST;
		goto put_parent;
	}

	if (!parent->i_op || !parent->i_op->mkdir || !parent->i_op->lookup) {
		res = -ENOSYS;
		goto put_parent;
	}

	struct dentry dentry = {
		.parent = parent_dentry,
		.name = name,
	};

	struct inode *child = parent->i_op->lookup(parent, &dentry);
	if (child) {
		if (IS_ERR(child)) {
			res = PTR_ERR(child);
			goto put_parent;
		}
		inode_put(child);

		res = -EEXIST;
		goto put_parent;
	}
	
	res = parent->i_op->mkdir(parent, &dentry, mode);

put_parent:
	inode_put(parent);
	dentry_put(parent_dentry);
	return res;
}

int vfs_rmdir(const char *restrict path){
	int res;
	struct qstr name;
	int trailing_slash = 0;
	struct dentry *parent_dentry = vfs_walk_parent(path, &name, &trailing_slash);
	if(IS_ERR_OR_NULL(parent_dentry)){
		return PTR_ERR(parent_dentry);
	}

	struct inode* parent = parent_dentry->inode;
	inode_get(parent);

	if (name.len == 1 && name.name[0] == '.') {
		res = -EINVAL;
		goto put_parent;
	}

	if (name.len == 2 && name.name[0] == '.' && name.name[1] == '.') {
		res = -ENOTEMPTY;
		goto put_parent;
	}

	if (!parent->i_op || !parent->i_op->rmdir || !parent->i_op->lookup) {
		res = -ENOSYS;
		goto put_parent;
	}

	struct dentry dentry = {
		.parent = parent_dentry,
		.name = name,
	};

	struct inode *child = parent->i_op->lookup(parent, &dentry);
	if (!child) {
		res = -ENOENT;
		goto put_parent;
	}

	if (IS_ERR(child)) {
		res = PTR_ERR(child);
		goto put_parent;
	}

	if (!S_ISDIR(child->mode)) {
		res = -ENOTDIR;
		goto put_child;
	}

	res = parent->i_op->rmdir(parent, &dentry);

put_child:
	inode_put(child);
put_parent:
	inode_put(parent);
	dentry_put(parent_dentry);
	return res;
}

int vfs_rename(const char *oldpath, const char *newpath) {
	struct qstr old_name, new_name;
	struct path src = {0}, dst = {0};

	int old_slash = 0, new_slash = 0, res = 0;
	struct dentry *old_d = NULL, *new_d = NULL;

	spin_lock(&rename_lock);

	struct dentry *old_parent = vfs_walk_parent(oldpath, &old_name, &old_slash);
	if (IS_ERR(old_parent)) {
		res = PTR_ERR(old_parent);
		goto out_unlock; 
	}

	struct dentry *new_parent = vfs_walk_parent(newpath, &new_name, &new_slash);
	if (IS_ERR(new_parent)) {
		res = PTR_ERR(new_parent);
		goto out_old_parent;
	}

	struct inode *old_dir = old_parent->inode;
	struct inode *new_dir = new_parent->inode;

	if (is_dot(&old_name) || is_dot(&new_name)) {
		res = -EINVAL;
	} else if (old_dir->i_sb != new_dir->i_sb) {
		res = -EXDEV;
	} else if (!old_dir->i_op || !old_dir->i_op->rename) {
		res = -ENOSYS;
	}

	if (res < 0) goto out_parents;

	res = vfs_lookup_path(old_parent, &old_name, &src);
	if (res) goto out_parents;

	old_d = src.dentry;
	if (src.mount) {
		res = -EBUSY;
		goto out;
	}

	const bool is_dir = S_ISDIR(old_d->inode->mode);
	if ((old_slash || new_slash) && !is_dir) { 
		res = -ENOTDIR;
		goto out;
	}

	res = vfs_lookup_path(new_parent, &new_name, &dst);
	if (res == 0) {
		new_d = dst.dentry;
		if (dst.mount) {
			res = -EBUSY;
			goto out;
		}
	} else if (res != -ENOENT) {
		goto out;
	}

	if (new_d == old_d) { 
		res = 0;
		goto out; 
	}

	if (new_d) {
		const bool new_is_dir = S_ISDIR(new_d->inode->mode);
		if (is_dir && !new_is_dir) {
			res = -ENOTDIR;
			goto out;
		}
		if (!is_dir && new_is_dir) {
			res = -EISDIR;
			goto out;
		}
	}

	if (is_dir) {
		for (struct dentry *p = new_parent; ; p = p->parent) {
			if (p == old_d) {
				res = -EINVAL;
				goto out;
			}
			if (p->parent == p) break;
		}
	}

	struct dentry new_stack = {
		.parent = new_parent,
		.name = new_name
	};

	res = old_dir->i_op->rename(old_dir, old_d, new_dir, &new_stack);

	if (res == 0) {
		if (new_d) d_drop(new_d);
		d_move(old_d, new_parent, &new_name);
	}

out:
	if (new_d) dentry_put(new_d);
	if (old_d) dentry_put(old_d);
out_parents:
	dentry_put(new_parent);
out_old_parent:
	dentry_put(old_parent);
out_unlock:
	spin_unlock(&rename_lock);
	return res;
}

static int path_copy(char* kpath, __user const char* upath, size_t maxlen){
	for(size_t i = 0; i < maxlen; i++){
		if(copy_from_user(kpath + i, upath + i, 1)){
			return -EFAULT;
		}

		if(!kpath[i]){
			return i+1;
		}
	}
	return -ENAMETOOLONG;
}

SYSCALL_DEFINE1(unlink, __user const char*, path){
	char kpath[PATH_MAX];

	int len = path_copy(kpath, path, PATH_MAX);
	if (len < 0) return len;

	return vfs_unlink(kpath);
}

SYSCALL_DEFINE2(link, __user const char*, oldpath, __user const char*, newpath){
	char kold[PATH_MAX], knew[PATH_MAX];

	int res = path_copy(kold, oldpath, PATH_MAX);
	if (res < 0) return res;

	res = path_copy(knew, newpath, PATH_MAX);
	if (res < 0) return res;

	return vfs_link(kold, knew);
}

SYSCALL_DEFINE2(mkdir, __user const char*, path, umode_t, mode){
	char kpath[PATH_MAX];

	int len = path_copy(kpath, path, PATH_MAX);
	if(len < 0) return len;

	return vfs_mkdir(kpath, mode);
}

SYSCALL_DEFINE1(rmdir, __user const char*, path){
	char kpath[PATH_MAX];

	int len = path_copy(kpath, path, PATH_MAX);
	if(len < 0) return len;

	return vfs_rmdir(kpath);
}

SYSCALL_DEFINE2(rename, __user const char*, oldpath, __user const char*, newpath) {
    char koldpath[PATH_MAX];
    char knewpath[PATH_MAX];

    int res = path_copy(koldpath, oldpath, PATH_MAX);
    if (res < 0) return res;

    res = path_copy(knewpath, newpath, PATH_MAX);
    if (res < 0) return res;

    return vfs_rename(koldpath, knewpath);
}
