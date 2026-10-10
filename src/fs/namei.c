#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/errno.h>
#include <fs/vfs.h>

extern struct mount *root_mount;

int vfs_create(const char *restrict path, umode_t mode){
	if(!root_mount) return -EINVAL;

	struct qstr name;
	struct inode* parent = vfs_walk_parent(path, &name);
	if(IS_ERR_OR_NULL(parent)){
		return PTR_ERR(parent);
	}

	if(!parent->i_op || !parent->i_op->create){
		inode_put(parent);
		return -ENOSYS;
	}

	int res = parent->i_op->create(parent, &name, mode);
	inode_put(parent);
	
	return res;
}

int vfs_mknod(const char *restrict path, umode_t mode, dev_t dev){
	if(!root_mount) return -EINVAL;

	struct qstr name;
	struct inode* parent = vfs_walk_parent(path, &name);
	if(IS_ERR_OR_NULL(parent)){
		return PTR_ERR(parent);
	}

	if(!parent->i_op || !parent->i_op->mknod){
		inode_put(parent);
		return -ENOSYS;
	}

	int res = parent->i_op->mknod(parent, &name, mode, dev);
	inode_put(parent);
	
	return res;
}

int vfs_unlink(const char *restrict path){
	if(!root_mount) return -EINVAL;

	struct qstr name;
	struct inode* parent = vfs_walk_parent(path, &name);
	if(IS_ERR_OR_NULL(parent)){
		return PTR_ERR(parent);
	}

	if(!parent->i_op || !parent->i_op->unlink){
		inode_put(parent);
		return -ENOSYS;
	}
	
	int res = parent->i_op->unlink(parent, &name);
	inode_put(parent);

	return res;
}

int vfs_mkdir(const char *restrict path){
	if(!root_mount) return -EINVAL;

	struct qstr name;
	struct inode* parent = vfs_walk_parent(path, &name);
	if(IS_ERR_OR_NULL(parent)){
		return PTR_ERR(parent);
	}

	if(!parent->i_op || !parent->i_op->mkdir){
		inode_put(parent);
		return -ENOSYS;
	}
	
	int res = parent->i_op->mkdir(parent, &name);
	inode_put(parent);

	return res;
}

int vfs_rmdir(const char *restrict path){
	if(!root_mount) return -EINVAL;

	struct qstr name;
	struct inode* parent = vfs_walk_parent(path, &name);
	if(IS_ERR_OR_NULL(parent)){
		return PTR_ERR(parent);
	}

	if(!parent->i_op || !parent->i_op->rmdir){
		inode_put(parent);
		return -ENOSYS;
	}
	
	int res = parent->i_op->rmdir(parent, &name);
	inode_put(parent);

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

SYSCALL_DEFINE2(mkdir, __user const char*, path, umode_t, mode){
	char kpath[PATH_MAX];
	size_t len = path_copy(kpath, path, PATH_MAX);
	if(IS_ERR_VALUE(len)){
		return len;
	}

	return vfs_mkdir(kpath);
}

SYSCALL_DEFINE1(rmdir, __user const char*, path){
	char kpath[PATH_MAX];
	size_t len = path_copy(kpath, path, PATH_MAX);
	if(IS_ERR_VALUE(len)){
		return len;
	}

	return vfs_rmdir(kpath);
}