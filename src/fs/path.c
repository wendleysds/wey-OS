#include <lib/string.h>
#include <def/config.h>
#include <def/errno.h>
#include <fs/vfs.h>
#include <fs/stat.h>

extern struct mount *root_mount;

int path_iterate(const char** cursor, struct qstr *comp, int *has_trailing_slash) {
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

	const char *lookahead = path;
	while (*lookahead == '/')
		lookahead++;

	if (*lookahead == '\0' && *path == '/') {
		if (has_trailing_slash) {
			*has_trailing_slash = 1;
		}
	} else {
		if (has_trailing_slash) {
			*has_trailing_slash = 0;
		}
	}

	*cursor = path;
	return 1;
}


int vfs_lookup_path(struct inode *parent, struct qstr *name, struct path *res){
	if (!parent->i_op || !parent->i_op->lookup)
		return -ENOSYS;

	struct inode *child = parent->i_op->lookup(parent, name);
	if (IS_ERR_OR_NULL(child))
		return child ? PTR_ERR(child) : -ENOENT;

	res->dentry = child;
	res->mount = NULL;

	spin_lock(&child->lock);
	if (child->mounted_here){
		res->mount = child->mounted_here;
	}
	spin_unlock(&child->lock);

	return SUCCESS;
}

int vfs_walk_path(const char *path, struct path *res) {
	if (!root_mount) return -EINVAL;

	struct mount *curr_mnt = root_mount;
	struct inode *curr_ino = root_mount->mnt_root;
	inode_get(curr_ino);

	const char *cursor = path;
	struct qstr comp;
	int trailing_slash = 0;

	while (path_iterate(&cursor, &comp, &trailing_slash)) {
		struct path next;
		int err = vfs_lookup_path(curr_ino, &comp, &next);
		if (err != SUCCESS) {
			inode_put(curr_ino);
			return err;
		}

		struct inode *next_ino = next.dentry;
		inode_put(curr_ino);
		curr_ino = next_ino;

		if (next.mount) {
			struct mount *next_mnt = next.mount;
			struct inode *mounted_root = next_mnt->mnt_root;

			inode_get(mounted_root);
			inode_put(curr_ino);
			curr_ino = mounted_root;
			curr_mnt = next_mnt;
		}
	}

	// POSIX: if path ends with slash, and the target is not a directory, return -ENOTDIR
	if (trailing_slash && !S_ISDIR(curr_ino->mode)) {
		inode_put(curr_ino);
		return -ENOTDIR;
	}

	res->mount = curr_mnt;
	res->dentry = curr_ino;
	return SUCCESS;
}

struct inode* vfs_walk_parent(const char *path, struct qstr *last, int *trailing_slash) {
	if(!root_mount) return ERR_PTR(-EINVAL);

	struct inode *cur = root_mount->mnt_root;
	inode_get(cur);

	struct qstr comp;
	const char* cursor = path;
	int has_slash = 0;

	while(path_iterate(&cursor, &comp, &has_slash)){
		const char* tmp = cursor;
		struct qstr next;
		int next_has_slash = 0;

		if(!path_iterate(&tmp, &next, &next_has_slash)){
			*last = comp;
			if (trailing_slash) {
				*trailing_slash = has_slash;
			}
			return cur;
		}

		struct path next_path;
		int err = vfs_lookup_path(cur, &comp, &next_path);
		if (err != SUCCESS) {
			inode_put(cur);
			return ERR_PTR(err);
		}

		struct inode *child = next_path.dentry;
		inode_put(cur);
		cur = child;

		if (next_path.mount) {
			struct inode *mounted_root = next_path.mount->mnt_root;
			inode_get(mounted_root);
			inode_put(cur);
			cur = mounted_root;
		}
	}

	inode_put(cur);
	return ERR_PTR(-EINVAL);
}

struct inode* vfs_walk(const char *path){
	if(!root_mount) return ERR_PTR(-EINVAL);

	struct path p;

	int res = vfs_walk_path(path, &p);

	if(IS_ERR_VALUE(res)){
		return ERR_PTR(res);
	}

	return p.dentry;
}