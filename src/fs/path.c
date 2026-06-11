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

struct inode* vfs_lookup(const char *restrict path){
	if(!root_mount) return NULL;

	const char* cursor = path;
	struct inode *child, *cur = root_mount->mnt_root;
	struct qstr comp;

	while(path_iterate(&cursor, &comp)){
		child = cur->i_op->lookup(cur, &comp);
		if(IS_ERR_OR_NULL(cur)){
			return cur;
		}

		spin_lock(&child->lock);

		if(child->mounted_here){
			child = child->mounted_here->mnt_root;
		}

		cur = child;

		spin_unlock(&child->lock);
	}

	return cur;
}
