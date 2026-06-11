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
		*cursor = NULL;
		return 0;
	}

	comp->name = path;

	while (*path != '/' && *path != '\0')
		path++;

	comp->len = path - comp->name;

	*cursor = (*path == '\0') ? NULL : path;

	return 1;
}

static struct inode* vfs_lookup_common(const char *restrict path, struct qstr* qstr_path){
	if(!root_mount) return ERR_PTR(INVALID_STATE);

	const char* end = qstr_path ? qstr_path->name + qstr_path->len : 0;
	const char* cursor = path ? path : qstr_path->name;
	struct inode *child, *cur = root_mount->mnt_root;
	struct qstr comp;

	inode_get(cur);

	while(path_iterate(&cursor, &comp)){
		if(end && comp.name > end) break;

		if(!cur->i_op || !cur->i_op->lookup){
			inode_put(cur);
			return ERR_PTR(NOT_SUPPORTED);
		}

		child = cur->i_op->lookup(cur, &comp);
		if(IS_ERR_OR_NULL(child)){
			inode_put(cur);
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

		inode_put(cur);
		cur = child;
	}

	return cur;
}

struct inode* vfs_lookup(const char *restrict path){
	return vfs_lookup_common(path, NULL);
}

struct inode* vfs_lookup_qstr(struct qstr* qstr_path){
	return vfs_lookup_common(NULL, qstr_path);
}
