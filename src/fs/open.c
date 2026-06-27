#include <def/err.h>
#include <fs/vfs.h>

struct file* vfs_open(const char *restrict path, int flags, umode_t mode){
	if(!path){
		return ERR_PTR(INVALID_ARG);
	}

	struct inode* ino = vfs_walk(path); // always return ino->refcount >= 1
	if(IS_ERR(ino)){
		if((PTR_ERR(ino) == FILE_NOT_FOUND || PTR_ERR(ino) == NO_ENTRY) && (flags & O_CREAT)){
			int res = vfs_create(path, mode);
			if(IS_ERR_VALUE(res)) return ERR_PTR(res);

			ino = vfs_walk(path);
			if(IS_ERR(ino)) return ERR_CAST(ino);
		}else{
			return ERR_CAST(ino);
		}
	}

	struct file* f = (struct file*)kmalloc(sizeof(struct file));
	if(!f){
		inode_put(ino);
		return ERR_PTR(NO_MEMORY);
	}

	if(flags & O_TRUNC){
		if(ino->i_op->setarrt){
			struct iattr iattr;
			iattr.valid = ATTR_SIZE;
			iattr.stat.size = 0;
			ino->i_op->setarrt(ino, &iattr);
		}
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
