#include <wey/vfs.h>
#include <def/err.h>
#include <mm/kheap.h>

struct file* vfs_open(const char *restrict path, uint32_t flags){
    if(!path){
        return ERR_PTR(INVALID_ARG);
    }

    struct inode* ino = vfs_lookup(path);
    if(IS_ERR(ino)){
        return ERR_PTR(PTR_ERR(ino));
    }

    struct file* f = (struct file*)kmalloc(sizeof(struct file));
    if(!f){
        return ERR_PTR(NO_MEMORY);
    }

    f->inode = ino;
    f->pos = 0;
    f->flags = flags;
    f->private_data = NULL;
    f->f_op = ino->i_fop;

    return f;
}

int vfs_close(struct file *file){
    if(!file){
        return INVALID_ARG;
    }

    if(!file->f_op || !file->f_op->close){
        return NOT_SUPPORTED;
    }

    int res = file->f_op->close(file);
    inode_destroy(file->inode);
    kfree(file);

    return res;
}