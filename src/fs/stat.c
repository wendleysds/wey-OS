#include <fs/vfs.h>
#include <def/err.h>

int vfs_getattr(const char *restrict path, struct stat *restrict statbuf){
    if(!path || !statbuf){
        return INVALID_ARG;
    }

    struct inode *ino = vfs_lookup(path);
    if(IS_ERR(ino)){
        return PTR_ERR(ino);
    }

    int res;

    if(!ino->i_op || !ino->i_op->getattr){
        res = NOT_SUPPORTED;
        goto out;
    }

    res = ino->i_op->getattr(ino, path, statbuf);
    
out:
    inode_dispose(ino);
    return res;
}