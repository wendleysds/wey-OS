#include <fs/vfs.h>
#include <mm/page.h>
#include <mm/kheap.h>
#include <def/errno.h>
#include <def/config.h>
#include <lib/string.h>
#include <stddef.h>

#include "ramfs_internal.h"

static ssize_t ramfs_read(struct file *file, void *buffer, size_t count, off_t *offset) {
	struct inode *inode = file->inode;
	struct ramfs_inode *rino = inode->private_data;

	spin_lock(&inode->lock);
	
	if (*offset >= (off_t)inode->size) {
		spin_unlock(&inode->lock);
		return 0;
	}

	size_t cursor = *offset;
	size_t cur_page_index = cursor / PAGE_SIZE;
	size_t page_offset = cursor % PAGE_SIZE;
	size_t total_readed = 0;
	size_t available = inode->size - cursor;

	if (count > available) {
		count = available;
	}

	size_t remaining = count;
	while (remaining > 0) {
		if (cur_page_index >= rino->page_capacity) {
			break;
		}

		struct page* page = rino->pages[cur_page_index];
		size_t available_in_page = PAGE_SIZE - page_offset;
		size_t to_read = (remaining < available_in_page) ? remaining : available_in_page;

		if (page) {
			char* page_ptr = (char*)page_to_virt(page);
			memcpy((char*)buffer + total_readed, page_ptr + page_offset, to_read);
		} else {
			memset((char*)buffer + total_readed, 0, to_read);
		}

		remaining -= to_read;
		total_readed += to_read;
		page_offset = 0;
		cur_page_index++;
	}

	spin_unlock(&inode->lock);

	*offset += total_readed;
	return total_readed;
}

static ssize_t ramfs_write(struct file *file, const void *buffer, size_t count, off_t *offset) {
	struct inode *inode = file->inode;
	struct ramfs_inode *rino = inode->private_data;

	if (count == 0) {
		return 0;
	}

	size_t cursor = *offset;
	size_t end_pos = cursor + count;
	size_t target_max_page_index = (end_pos - 1) / PAGE_SIZE;

	spin_lock(&inode->lock);
	if (target_max_page_index >= rino->page_capacity) {
		size_t new_cap = rino->page_capacity ? rino->page_capacity : 4;
		while (target_max_page_index >= new_cap) {
			new_cap *= 2;
		}
		
		int res = ramfs_grow_pages_arr(rino, new_cap);
		if (res) {
			spin_unlock(&inode->lock);
			return -ENOMEM;
		}
	}
	spin_unlock(&inode->lock);

	size_t cur_page_index = cursor / PAGE_SIZE;
	size_t page_offset = cursor % PAGE_SIZE;
	size_t remaining = count;
	size_t total_written = 0;

	while (remaining > 0) {
		spin_lock(&inode->lock);
		struct page *page = rino->pages[cur_page_index];
		spin_unlock(&inode->lock);

		if (!page) {
			struct page *new_page = page_alloc(0, PG_KERNEL);
			if (!new_page) {
				return total_written ? (ssize_t)total_written : -ENOMEM;
			}
			memset((void*)page_to_virt(new_page), 0, PAGE_SIZE);

			spin_lock(&inode->lock);
			if (!rino->pages[cur_page_index]) {
				rino->pages[cur_page_index] = new_page;
				page = new_page;
			} else {
				page = rino->pages[cur_page_index];
				page_free(new_page);
			}
			spin_unlock(&inode->lock);
		}

		char *page_ptr = (char*)page_to_virt(page);
		size_t available_in_page = PAGE_SIZE - page_offset;
		size_t to_write = (remaining < available_in_page) ? remaining : available_in_page;

		memcpy(page_ptr + page_offset, (const char*)buffer + total_written, to_write);

		remaining -= to_write;
		total_written += to_write;
		page_offset = 0;
		cur_page_index++;
	}

	spin_lock(&inode->lock);
	cursor += total_written;
	if (cursor > inode->size) {
		inode->size = cursor;
	}
	spin_unlock(&inode->lock);

	*offset = cursor;

	return total_written;
}


off_t ramfs_lseek(struct file *file, off_t offset, int whence){
	size_t filesize = file->inode->size;

	off_t target;
	switch(whence){
		case SEEK_SET:
			target = offset;
			break;
		case SEEK_CUR:
			target = (off_t)file->pos + offset;
			break;
		case SEEK_END:
			target = (off_t)filesize + offset;
			break;
		default:
			return -EINVAL;
	}

	if(target < 0){
		return -EINVAL;
	}

	if(target > (off_t)filesize){
		int res = ramfs_do_truncate(file->inode, target);
		if(IS_ERR_VALUE(res)){
			return res;
		}
	}

	file->pos = (size_t)target;
	return target;
}

const struct file_operations ramfs_fops = {
	.read = ramfs_read,
	.write = ramfs_write,
	.lseek = ramfs_lseek,
};