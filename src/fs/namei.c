#include <lib/string.h>
#include <def/config.h>
#include <def/errno.h>
#include <fs/vfs.h>

extern struct mount *root_mount;

int vfs_create(const char *restrict path, uint16_t mode){
	if(!root_mount) return -EINVAL;

	struct qstr name;
	struct inode* parent = vfs_walk_parent(path, &name);
	if(IS_ERR_OR_NULL(parent)){
		return PTR_ERR(parent);
	}

	int res = parent->i_op->create(parent, &name, mode);
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
	
	int res = parent->i_op->rmdir(parent, &name);
	inode_put(parent);

	return res;
}
