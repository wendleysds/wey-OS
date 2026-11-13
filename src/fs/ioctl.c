#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <fs/fdtable.h>
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

SYSCALL_DEFINE3(ioctl, unsigned int, fd, unsigned int, cmd, unsigned long, arg){
    struct file *file = file_table_get_file(current->files, fd);
    if(!file){
        return -EBADF;
    }

    int ret = vfs_ioctl(file, cmd, arg);
    file_put(file);

    return ret;
}
