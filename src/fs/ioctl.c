#include <fs/stat.h>
#include <fs/vfs.h>
#include <def/errno.h>

int vfs_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    const struct inode *inode = file->inode;
    if(!S_ISREG(inode->mode)){
        return -ENOTTY;
    }

    if(!file->f_op || !file->f_op->ioctl){
        return -ENOTTY;
    }

	return file->f_op->ioctl(file, cmd, arg);
}
