#include <def/err.h>
#include <fs/vfs.h>

struct file* vfs_open(const char *restrict path, uint32_t flags){
	if(!path){
		return ERR_PTR(INVALID_ARG);
	}

	struct inode* ino = vfs_lookup(path); // always return ino->refcount >= 1
	if(IS_ERR(ino)){
		return ERR_PTR(PTR_ERR(ino));
	}

	struct file* f = (struct file*)kmalloc(sizeof(struct file));
	if(!f){
		inode_put(ino);
		return ERR_PTR(NO_MEMORY);
	}

	if(atomic_read(&ino->refcount) > 1){
		inode_put(ino);
	}

	f->inode = ino;
	f->pos = 0;
	f->flags = flags;
	f->private_data = NULL;
	f->f_op = ino->i_fop;

	atomic_set(&f->refcount, 1);

	if (f->f_op && f->f_op->open) {
		int ret = f->f_op->open(ino, f);
		if (ret) {
			file_put(f);
			return ERR_PTR(ret);
		}
	}

	return f;
}

int vfs_close(struct file *file){
	if(!file){
		return INVALID_ARG;
	}

	int res = SUCCESS;

	if(file->f_op && file->f_op->close){
		res = file->f_op->close(file);
	}

	if(!IS_ERR_VALUE(res)){
		file_put(file);
	}

	return res;
}
