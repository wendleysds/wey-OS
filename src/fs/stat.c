#include <lib/string.h>
#include <def/config.h>
#include <def/errno.h>
#include <fs/vfs.h>
#include <fs/dcache.h>

int vfs_getattr(const char *restrict path, struct stat *restrict statbuf){
    if(!path || !statbuf){
        return -EINVAL;
    }

    struct dentry *d = vfs_walk(path);
    if(IS_ERR(d)){
        return PTR_ERR(d);
    }

    struct inode *ino = d->inode;
    inode_get(ino);
    dentry_put(d);

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
