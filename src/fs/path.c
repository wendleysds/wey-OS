#include <lib/string.h>
#include <def/config.h>
#include <def/err.h>
#include <fs/vfs.h>

extern struct mount *root_mount;

int path_iterate(const char** cursor, struct qstr* comp){
	const char* path = *cursor;

	while (*path == '/')
		path++;

	if (*path == '\0') {
		return 0;
	}

	comp->name = path;

	while (*path != '/' && *path != '\0')
		path++;

	comp->len = path - comp->name;

	*cursor = path;

	return 1;
}

struct inode* vfs_lookup(struct inode *parent, struct qstr *name){
	if(!parent->i_op || !parent->i_op->lookup){
		return ERR_PTR(NOT_SUPPORTED);
	}

	struct inode* child = parent->i_op->lookup(parent, name);
	if(IS_ERR_OR_NULL(child)){
		return child ? child : ERR_PTR(NO_ENTRY);
	}

	struct inode *mounted_root = NULL;

	spin_lock(&child->lock);
	if(child->mounted_here){
		mounted_root = child->mounted_here->mnt_root;
		inode_get(mounted_root);
	}
	spin_unlock(&child->lock);

	if(mounted_root){
		inode_put(child);
		child = mounted_root;
	}

	return child;
}

int vfs_lookup_path(struct inode *parent, struct qstr *name, struct path *res){
	if (!parent->i_op || !parent->i_op->lookup) {
		return NOT_SUPPORTED;
	}

	struct inode *child = parent->i_op->lookup(parent, name);
	if (IS_ERR_OR_NULL(child)) {
		return child ? PTR_ERR(child) : NO_ENTRY;
	}

	spin_lock(&child->lock);
	if (child->mounted_here) {
		struct mount *next_mnt = child->mounted_here;
		struct inode *mounted_root = next_mnt->mnt_root;
		
		inode_get(mounted_root);
		inode_put(child);

		child = mounted_root;
		res->mount = next_mnt;
	}
	spin_unlock(&child->lock);

	res->dentry = child;
	return OK;
}

int vfs_walk_path(const char *path, struct path *res) {
	if (!root_mount) return INVALID_STATE;

	struct mount *curr_mnt = root_mount;
	struct inode *curr_ino = root_mount->mnt_root;
	inode_get(curr_ino);

	const char *cursor = path;
	struct qstr comp;

	while (path_iterate(&cursor, &comp)) {
		int ret = vfs_lookup_path(curr_ino, &comp, res);
		inode_put(curr_ino);
	
		if(IS_ERR_VALUE(ret)){
			return ret;
		}

		curr_ino = res->dentry;
	}

	res->mount = curr_mnt;
	res->dentry = curr_ino;
	return SUCCESS;
}

struct inode* vfs_walk_parent(const char *path, struct qstr *last){
	if(!root_mount) return ERR_PTR(INVALID_STATE);

	struct inode *cur = root_mount->mnt_root;
	inode_get(cur);

	struct qstr comp;
	const char* cursor = path;
	while(path_iterate(&cursor, &comp)){
		const char* tmp = cursor;
		struct qstr next;

		if(!path_iterate(&tmp, &next)){
			*last = comp;
			return cur;
		}

		struct inode* child = vfs_lookup(cur, &comp);
		if(IS_ERR(child)){
			inode_put(cur);
			return child;
		}

		inode_put(cur);
		cur = child;
	}

	inode_put(cur);
	return ERR_PTR(INVALID_ARG);
}

struct inode* vfs_walk(const char *path){
	if(!root_mount) return ERR_PTR(INVALID_STATE);

	struct path p;

	int res = vfs_walk_path(path, &p);

	if(IS_ERR_VALUE(res)){
		return ERR_PTR(res);
	}

	return p.dentry;
}