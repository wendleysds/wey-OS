#include <lib/string.h>
#include <def/config.h>
#include <def/errno.h>
#include <fs/vfs.h>

int vfs_getattr(const char *restrict path, struct stat *restrict statbuf){
    if(!path || !statbuf){
        return -EINVAL;
    }

    struct inode *ino = vfs_walk(path);
    if(IS_ERR(ino)){
        return PTR_ERR(ino);
    }

    int res;

    if(!ino->i_op || !ino->i_op->getattr){
        res = -ENOSYS;
        goto out;
    }

    res = ino->i_op->getattr(ino, statbuf);

out:
    inode_put(ino);
    return res;
}
