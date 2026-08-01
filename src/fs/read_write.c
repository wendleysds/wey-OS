#include <fs/vfs.h>
#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/uaccess.h>
#include <def/config.h>
#include <def/errno.h>

static int get_proc_file(int fd, struct file **out){
	if(fd < 0 || fd > PROC_FD_MAX){
		return -EBADF;
	}

	*out = current->file_table[fd];
	if(!*out){
		return -EBADF;
	}

	return 0;
}

static int validate_file(const struct file *file){
	return (!file || !file->f_op) ? -EIO : 0;
}

int vfs_read(struct file *file, void *buffer, uint32_t size){
	int ret = validate_file(file);
	if(ret){
		return ret;
	}

	if(!file->f_op->read){
		return -ENOSYS;
	}
	
	return file->f_op->read(file, buffer, size);
}

int vfs_write(struct file *file, const void *buffer, uint32_t size){
	int ret = validate_file(file);
	if(ret){
		return ret;
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

SYSCALL_DEFINE3(write, int, fd, const __user char*, buffer, size_t, count){
	struct file *file;
	int ret = get_proc_file(fd, &file);
	if(ret){
		return ret;
	}

	if((unsigned long)(buffer + count) > USER_SPACE_END){
		return -EFAULT;
	}

	if(!file->f_op->write){
		return -ENOSYS;
	}

	char tmp[512];
	size_t total_written = 0;

	while(count > 0){
		size_t to_copy = (count > sizeof(tmp)) ? sizeof(tmp) : count;

		int remaining = copy_from_user(tmp, buffer + total_written, to_copy);
		if(remaining){
			if(total_written == 0 && remaining == to_copy){
				return -EFAULT;
			}

			total_written += to_copy - remaining;
			break;
		}

		int ret = vfs_write(file, tmp, to_copy);
		if(ret < 0){
			return ret;
		}

		if(ret == 0){
			break;
		}

		total_written += to_copy;

		if((size_t)ret != to_copy){
			break;
		}

		count -= to_copy;
	}

	return total_written;
}

SYSCALL_DEFINE3(read, int, fd, __user char*, buffer, size_t, count){
	struct file *file;
	int ret = get_proc_file(fd, &file);
	if(ret){
		return ret;
	}

	if(!buffer || (unsigned long)(buffer + count) > USER_SPACE_END){
		return -EFAULT;
	}

	if(!file->f_op->read){
		return -ENOSYS;
	}

	char tmp[512];
	size_t total_read = 0;

	while(count > 0){
		size_t to_read = (count > sizeof(tmp)) ? sizeof(tmp) : count;
		int readed = vfs_read(file, tmp, to_read);
		if(readed < 0){
			return readed;
		}

		if(readed == 0){
			break;
		}

		int remaining = copy_to_user(buffer + total_read, tmp, readed);
		if(remaining){
			if(total_read == 0 && remaining == readed){
				return -EFAULT;
			}

			total_read += readed - remaining;
			break;
		}

		total_read += readed;
		count -= readed;

		if((size_t)readed != to_read){
			break;
		}
	}

	return total_read;
}
