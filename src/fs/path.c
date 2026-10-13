#include <kernel/sched.h>
#include <fs/vfs.h>
#include <fs/stat.h>
#include <fs/dcache.h>
#include <def/config.h>
#include <def/errno.h>
#include <lib/string.h>

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

int vfs_lookup_path(struct dentry *parent, struct qstr *name, struct path *res){
	if (!parent->inode->i_op || !parent->inode->i_op->lookup)
		return -ENOSYS;

	struct dentry *child = dcache_lookup(parent, name);
	if (!child) {
		struct dentry dentry = {
			.parent = parent,
			.name = *name,
		};
		struct inode *ino = parent->inode->i_op->lookup(parent->inode, &dentry);
		if (IS_ERR_OR_NULL(ino))
			return ino ? PTR_ERR(ino) : -ENOENT;

		child = dcache_add(parent, name, ino);
		inode_put(ino);
		if (!child){
			return -ENOMEM;
		}
	}

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

	struct mount *curr_mnt;
    struct dentry *curr_dentry;

    if (path[0] == '/') {
        curr_mnt = root_mount;
        curr_dentry = root_mount->mnt_root;
    } else {
        if (current && current->cwd.dentry) {
            curr_dentry = current->cwd.dentry;
            curr_mnt = current->cwd.mount;
        } else {
            curr_mnt = root_mount;
            curr_dentry = root_mount->mnt_root;
        }
    }

	dentry_get(curr_dentry);

	const char *cursor = path;
	struct qstr comp;
	int trailing_slash = 0;

	while (path_iterate(&cursor, &comp, &trailing_slash)) {
		if (comp.len == 1 && comp.name[0] == '.') {
			continue;
		}

		if (comp.len == 2 && comp.name[0] == '.' && comp.name[1] == '.') {
			if (curr_dentry == root_mount->mnt_root) {
				continue;
			}

			if (curr_dentry == curr_mnt->mnt_root && curr_mnt != root_mount) {
				struct mount *parent_mnt = curr_mnt->parent;
				struct dentry *mountpoint = curr_mnt->mnt_mountpoint;
				struct dentry *parent_dentry = dentry_get(mountpoint->parent);
				dentry_put(curr_dentry);
				curr_dentry = parent_dentry;
				curr_mnt = parent_mnt;
				continue;
			}

			struct dentry *parent_dentry = dentry_get(curr_dentry->parent);
			dentry_put(curr_dentry);
			curr_dentry = parent_dentry;
			continue;
		}

		struct path next;
		int err = vfs_lookup_path(curr_dentry, &comp, &next);
		if (err != SUCCESS) {
			dentry_put(curr_dentry);
			return err;
		}

		struct dentry *next_dentry = next.dentry;
		dentry_put(curr_dentry);
		curr_dentry = next_dentry;

		if (next.mount) {
			struct mount *next_mnt = next.mount;
			struct dentry *mounted_root = next_mnt->mnt_root;

			dentry_get(mounted_root);
			dentry_put(curr_dentry);
			curr_dentry = mounted_root;
			curr_mnt = next_mnt;
		}
	}

	// POSIX: if path ends with slash, and the target is not a directory, return -ENOTDIR
	if (trailing_slash && !S_ISDIR(curr_dentry->inode->mode)) {
		dentry_put(curr_dentry);
		return -ENOTDIR;
	}

	res->mount = curr_mnt;
	res->dentry = curr_dentry;
	return SUCCESS;
}

struct dentry* vfs_walk_parent(const char *path, struct qstr *last, int *trailing_slash) {
	if(!root_mount) return ERR_PTR(-EINVAL);

	struct dentry *cur;
	if (path[0] == '/') {
		cur = root_mount->mnt_root;
	} else if (current && current->cwd.dentry) {
		cur = current->cwd.dentry;
	} else {
		cur = root_mount->mnt_root;
	}
	dentry_get(cur);

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

		if (comp.len == 1 && comp.name[0] == '.') {
			continue;
		}

		if (comp.len == 2 && comp.name[0] == '.' && comp.name[1] == '.') {
			if (cur == root_mount->mnt_root) {
				continue;
			}
			if (cur == cur->parent) {
				continue;
			}
			struct dentry *parent = dentry_get(cur->parent);
			dentry_put(cur);
			cur = parent;
			continue;
		}

		struct path next_path;
		int err = vfs_lookup_path(cur, &comp, &next_path);
		if (err != SUCCESS) {
			dentry_put(cur);
			return ERR_PTR(err);
		}

		struct dentry *child = next_path.dentry;
		dentry_put(cur);
		cur = child;

		if (next_path.mount) {
			struct dentry *mounted_root = next_path.mount->mnt_root;
			dentry_get(mounted_root);
			dentry_put(cur);
			cur = mounted_root;
		}
	}

	dentry_put(cur);
	return ERR_PTR(-EINVAL);
}

struct dentry* vfs_walk(const char *path){
	if(!root_mount) return ERR_PTR(-EINVAL);

	struct path p;

	int res = vfs_walk_path(path, &p);

	if(IS_ERR_VALUE(res)){
		return ERR_PTR(res);
	}

	return p.dentry;
}
