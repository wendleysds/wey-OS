#include <wey/vfs.h>
#include <def/err.h>
#include <def/config.h>
#include <lib/string.h>

int vfs_getattr(const char *restrict path, struct stat *restrict statbuf){
    if(!path || !statbuf){
        return INVALID_ARG;
    }

	char pathcopy[PATH_MAX];
	strcpy(pathcopy, path);

	char* slash = strrchr(pathcopy, '/');
	*slash = '\0';

	char* filename = slash + 1;
	char* dirs = pathcopy;

    struct inode *dir = vfs_lookup(strlen(dirs) == 0 ? "/" : dirs);
    if(IS_ERR(dir)){
        return PTR_ERR(dir);
    }

    int res;

    if(!dir->i_op || !dir->i_op->getattr){
        res = NOT_SUPPORTED;
        goto out;
    }

    res = dir->i_op->getattr(dir, filename, statbuf);
    
out:
    inode_destroy(dir);
    return res;
}