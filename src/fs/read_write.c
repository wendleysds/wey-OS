#include <fs/vfs.h>
#include <fs/fdtable.h>
#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/uaccess.h>
#include <def/config.h>
#include <def/errno.h>

static int validate_file(const struct file *file){
	return (!file || !file->f_op) ? -EIO : 0;
}

ssize_t vfs_read(struct file *file, void *buffer, size_t size){
	int ret = validate_file(file);
	if(ret){
		return ret;
	}

	if(!file->f_op->read){
		return -ENOSYS;
	}
	
	return file->f_op->read(file, buffer, size, &file->pos);
}

ssize_t vfs_pread(struct file *file, void *buffer, size_t count, off_t offset){
	int ret = validate_file(file);
	if(ret){
		return ret;
	}
	
	if(!file->f_op->read){
		return -ENOSYS;
	}
	
	off_t local_offset = offset;
	return file->f_op->read(file, buffer, count, &local_offset);
}

ssize_t vfs_write(struct file *file, const void *buffer, size_t size){
	int ret = validate_file(file);
	if(ret){
		return ret;
	}

	if(!file->f_op->write){
		return -ENOSYS;
	}
	
	return file->f_op->write(file, buffer, size, &file->pos);
}

ssize_t vfs_pwrite(struct file *file, const void *buffer, size_t count, off_t offset){
	int ret = validate_file(file);
	if(ret){
		return ret;
	}

	if(!file->f_op->write){
		return -ENOSYS;
	}
	
	off_t local_offset = offset;
	return file->f_op->write(file, buffer, count, &local_offset);
}

off_t vfs_lseek(struct file *file, off_t offset, int whence){
	if(!file || !file->f_op || !file->f_op->lseek){
		return -ENOSYS;
	}
	
	return file->f_op->lseek(file, offset, whence);
}

SYSCALL_DEFINE3(write, int, fd, const __user char*, buffer, size_t, count){
	struct file *file = file_table_get_file(current->files, fd);
	if(!file){
		return -EBADF;
	}

	if((unsigned long)(buffer + count) > USER_SPACE_END){
		file_put(file);
		return -EFAULT;
	}

	if(!file->f_op || !file->f_op->write){
		file_put(file);
		return -ENOSYS;
	}

	char tmp[512];
	size_t total_written = 0;

	while (total_written < count) {
		size_t remaining = count - total_written;
		size_t to_copy = remaining > sizeof(tmp)
					? sizeof(tmp)
					: remaining;

		int not_copied =
			copy_from_user(tmp,
						buffer + total_written,
						to_copy);

		if (not_copied) {
			size_t copied = to_copy - not_copied;

			if (copied == 0 && total_written == 0) {
				file_put(file);
				return -EFAULT;
			}

			total_written += copied;
			break;
		}

		ssize_t ret = vfs_write(file, tmp, to_copy);

		if (ret < 0) {
			if (total_written != 0)
				break;

			file_put(file);
			return ret;
		}

		if (ret == 0)
			break;

		total_written += ret;

		if ((size_t)ret < to_copy)
			break;
	}

	file_put(file);
	return total_written;
}

SYSCALL_DEFINE3(read, int, fd, __user char*, buffer, size_t, count){
	struct file *file = file_table_get_file(current->files, fd);
	if(!file){
		return -EBADF;
	}

	if(!buffer || (unsigned long)(buffer + count) > USER_SPACE_END){
		file_put(file);
		return -EFAULT;
	}

	if(!file->f_op || !file->f_op->read){
		file_put(file);
		return -ENOSYS;
	}

	char tmp[512];
	size_t total_read = 0;

	while (total_read < count) {
		size_t remaining = count - total_read;
		size_t chunk = remaining > sizeof(tmp)
					? sizeof(tmp)
					: remaining;

		ssize_t n = vfs_read(file,
							tmp,
							chunk);

		if (n < 0) {
			if (total_read)
				break;

			file_put(file);
			return n;
		}

		if (n == 0)
			break;

		size_t not_copied =
			copy_to_user(buffer + total_read, tmp, n);

		size_t copied = n - not_copied;

		total_read += copied;

		if (not_copied)
			break;

		if ((size_t)n < chunk)
			break;
	}

	file_put(file);
	return total_read;
}

SYSCALL_DEFINE3(lseek, int, fd, int, offset, int, whence){
	struct file *file = file_table_get_file(current->files, fd);
	if(!file){
		return -EBADF;
	}

	int res = vfs_lseek(file, offset, whence);
	file_put(file);

	return res;
}