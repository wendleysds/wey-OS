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

struct inode* vfs_walk_parent(const char *path, struct qstr *last){
	if(!root_mount) return ERR_PTR(INVALID_STATE);

	struct inode *cur = root_mount->mnt_root;
	inode_get(cur);

	const char* cursor = path;
	struct qstr comp;

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

	struct inode *next, *cur = root_mount->mnt_root;
	inode_get(cur);

	if(path[1] == '\0') return cur;

	const char* cursor = path;
	struct qstr comp;

	while(path_iterate(&cursor, &comp)){
		next = vfs_lookup(cur, &comp);

		if(IS_ERR(next)){
			inode_put(cur);
			return next;
		}

		inode_put(cur);
		cur = next;
	}

	return cur;
}

