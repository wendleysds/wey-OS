#include <def/errno.h>
#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/uaccess.h>
#include <fs/fdtable.h>
#include <fs/vfs.h>

struct file* vfs_open(const char *restrict path, int flags, umode_t mode){
	if(!path){
		return ERR_PTR(-EINVAL);
	}

	struct inode* ino = vfs_walk(path); // always return ino->refcount >= 1
	if(IS_ERR(ino)){
		if((PTR_ERR(ino) == -ENOENT || PTR_ERR(ino) == -ENOENT) && (flags & O_CREAT)){
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
		return ERR_PTR(-ENOMEM);
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
		return -EINVAL;
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

SYSCALL_DEFINE3(open, __user const char *restrict, path, int, flags, umode_t, mode){
	char kpath[PATH_MAX];
	int ret = copy_from_user(kpath, path, PATH_MAX);
	if(ret){
		return -EFAULT;
	}
	
	struct file* f = vfs_open(kpath, flags, mode);
	if(IS_ERR(f)){
		return PTR_ERR(f);
	}

	f->pos = 0;

	int fd = task_add_file(current, f);
	file_put(f); // file_table now holds a reference, so we can release ours

	if(IS_ERR_VALUE(fd)){
		return fd;
	}

	return fd;
}

SYSCALL_DEFINE1(close, int, fd){
	int res = task_remove_file(current, fd);

	if(IS_ERR_VALUE(res)){
		return res;
	}

	return SUCCESS;
}
