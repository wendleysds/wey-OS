#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <fs/fdtable.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <def/errno.h>

static int check_file_ioctl(struct file *file, unsigned int fd, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        default:
            if (S_ISREG(file->inode->mode)) {
            }
            break;
    }

    return -ENOIOCTLCMD;
}

int vfs_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    if (!file->f_op || !file->f_op->ioctl) {
        return -ENOTTY;
    }

    int error = file->f_op->ioctl(file, cmd, arg);
    if (error == -ENOIOCTLCMD) {
        return -ENOTTY;
    }

    return error;
}

SYSCALL_DEFINE3(ioctl, unsigned int, fd, unsigned int, cmd, unsigned long, arg){
    struct file *file = file_table_get_file(current->files, fd);
    if(!file){
        return -EBADF;
    }

    int ret = check_file_ioctl(file, fd, cmd, arg);
	if (ret == -ENOIOCTLCMD)
		ret = vfs_ioctl(file, cmd, arg);

    return ret;
}
