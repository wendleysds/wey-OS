#include <def/errno.h>
#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/uaccess.h>
#include <fs/fdtable.h>
#include <fs/vfs.h>
#include <fs/stat.h>
#include <fs/dcache.h>

extern struct mount *root_mount;

struct file* vfs_open(const char *restrict path, int flags, umode_t mode) {
	if (!path) return ERR_PTR(-EINVAL);

	struct inode *parent = NULL;
	struct dentry *parent_dentry = NULL;

	struct inode *ino = NULL;

	struct qstr last;
	int trailing_slash = 0;

	if (flags & O_CREAT) {
		if (flags & O_DIRECTORY) {
			return ERR_PTR(-ENOENT);
		}

		parent_dentry = vfs_walk_parent(path, &last, &trailing_slash);
		if (IS_ERR(parent_dentry)) {

			// Edge case: opening "/" as a directory
			if (PTR_ERR(parent_dentry) == -EINVAL && path[0] == '/' && path[1] == '\0') {
				ino = root_mount->mnt_root->inode;
				inode_get(ino);
				if (flags & O_EXCL) { 
					inode_put(ino); 
					return ERR_PTR(-EEXIST); 
				}
			} else {
				return ERR_CAST(parent);
			}
		} else {
			if (trailing_slash) {
				inode_put(parent);
				return ERR_PTR(-ENOENT);
			}

			parent = parent_dentry->inode;
			inode_get(parent);
			dentry_put(parent_dentry);

			ino = parent->i_op->lookup(parent, &last);
			if (IS_ERR(ino)) {
				inode_put(parent);
				return ERR_CAST(ino);
			}

			if (ino == NULL) {
				if (!parent->i_op->create) {
					inode_put(parent);
					return ERR_PTR(-ENOSYS);
				}
				
				int res = parent->i_op->create(parent, &last, mode);
				inode_put(parent);
				
				if (res < 0) return ERR_PTR(res);

				struct dentry *ino_dentry = vfs_walk(path); 
				if (IS_ERR(ino_dentry)) return ERR_CAST(ino_dentry);

				ino = ino_dentry->inode;
				inode_get(ino);
				dentry_put(ino_dentry);
			} else {
				inode_put(parent);
				if (flags & O_EXCL) {
					inode_put(ino);
					return ERR_PTR(-EEXIST);
				}
			}
		}
	} else {
		struct dentry *ino_dentry = vfs_walk(path);
		if (IS_ERR(ino_dentry)) return ERR_CAST(ino_dentry);

		ino = ino_dentry->inode;
		inode_get(ino);
		dentry_put(ino_dentry);
	}

	if ((flags & O_DIRECTORY) && !S_ISDIR(ino->mode)) {
		inode_put(ino);
		return ERR_PTR(-ENOTDIR);
	}

	if (S_ISDIR(ino->mode) && (flags & (O_WRONLY | O_RDWR))) {
		inode_put(ino);
		return ERR_PTR(-EISDIR);
	}

	if (flags & O_TRUNC) {
		if (S_ISDIR(ino->mode)) {
			inode_put(ino);
			return ERR_PTR(-EISDIR);
		}
		if (ino->i_op && ino->i_op->setattr) {
			struct iattr iattr;
			iattr.valid = ATTR_SIZE;
			iattr.stat.size = 0;
			ino->i_op->setattr(ino, &iattr);
		}
	}

	struct file* f = (struct file*)kmalloc(sizeof(struct file));
	if (!f) {
		inode_put(ino);
		return ERR_PTR(-ENOMEM);
	}

	f->inode = ino;
	f->pos = 0;
	f->flags = flags;
	f->private_data = NULL;
	f->f_op = ino->i_fop;
	atomic_set(&f->refcount, 1);

	if (f->f_op && f->f_op->open) {
		int ret = f->f_op->open(ino, f);
		if (ret) {
			file_put(f);
			return ERR_PTR(ret);
		}
	}

	if(flags & O_APPEND){
		f->pos = f->inode->size;
	}

	return f;
}

int vfs_close(struct file *file){
	if(!file){
		return -EINVAL;
	}

	int res = SUCCESS;

	if(file->f_op && file->f_op->close){
		res = file->f_op->close(file);
	}

	if(!IS_ERR_VALUE(res)){
		file_put(file);
	}

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

SYSCALL_DEFINE3(open, __user const char *restrict, path, int, flags, umode_t, mode){
	char kpath[PATH_MAX];
	size_t len = path_copy(kpath, path, PATH_MAX);
	if(IS_ERR_VALUE(len)){
		return len;
	}
	
	struct file* f = vfs_open(kpath, flags, mode);
	if(IS_ERR(f)){
		return PTR_ERR(f);
	}

	f->pos = 0;

	int fd = task_add_file(current, f);
	file_put(f); // file_table now holds a reference, so we can release ours

	if(IS_ERR_VALUE(fd)){
		return fd;
	}

	return fd;
}

SYSCALL_DEFINE1(close, int, fd){
	int res = task_remove_file(current, fd);

	if(IS_ERR_VALUE(res)){
		return res;
	}

	return SUCCESS;
}
