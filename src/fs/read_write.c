#include <wey/vfs.h>
#include <def/status.h>

int vfs_read(struct file *file, void *buffer, uint32_t size){
    if(!file || !file->f_op){
        return READ_FAIL;
    }

    if(!file->f_op->read){
        return NOT_SUPPORTED;
    }
    
    return file->f_op->read(file, buffer, size);
}

int vfs_write(struct file *file, const void *buffer, uint32_t size){
    if(!file || !file->f_op){
        return WRITE_FAIL;
    }

    if(!file->f_op->write){
        return NOT_SUPPORTED;
    }
    
    return file->f_op->write(file, buffer, size);
}

int vfs_lseek(struct file *file, int offset, int whence){
    if(!file || !file->f_op || !file->f_op->lseek){
        return NOT_SUPPORTED;
    }
    
    return file->f_op->lseek(file, offset, whence);
}
