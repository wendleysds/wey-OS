#include <fs/vfs.h>
#include <def/errno.h>

int vfs_read(struct file *file, void *buffer, uint32_t size){
    if(!file || !file->f_op){
        return -EIO;
    }

    if(!file->f_op->read){
        return -ENOSYS;
    }
    
    return file->f_op->read(file, buffer, size);
}

int vfs_write(struct file *file, const void *buffer, uint32_t size){
    if(!file || !file->f_op){
        return -EIO;
    }

    if(!file->f_op->write){
        return -ENOSYS;
    }
    
    return file->f_op->write(file, buffer, size);
}

int vfs_lseek(struct file *file, int offset, int whence){
    if(!file || !file->f_op || !file->f_op->lseek){
        return -ENOSYS;
    }
    
    return file->f_op->lseek(file, offset, whence);
}
