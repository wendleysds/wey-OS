#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/errno.h>
#include <fs/vfs.h>
#include <fs/dcache.h>

#include <uapi/sys/stat.h>

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

SYSCALL_DEFINE2(stat, __user const char *restrict, pathname, __user struct stat *restrict, statbuf){
    struct stat kstat;
    char kpath[PATH_MAX];

	int ret = path_copy_from_user(kpath, pathname, PATH_MAX);
	if(ret < 0) return ret;

    ret = vfs_getattr(kpath, &kstat);
    if(ret) {
        return ret;
    }

    ret = copy_to_user(statbuf, &kstat, sizeof(*statbuf));
    return ret ? -EFAULT : 0;
}
