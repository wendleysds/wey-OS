#include <fs/vfs.h>
#include <mm/page.h>
#include <mm/kheap.h>
#include <def/err.h>
#include <def/config.h>
#include <lib/string.h>
#include <stddef.h>

#include "ramfs_internal.h"

static int ramfs_read(struct file *file, void *buffer, uint32_t count){
	struct inode *inode = file->inode;
	struct ramfs_inode *rino = inode->private_data;

	if(file->pos >= inode->size){
		return END_OF_FILE;
	}

	size_t cursor = file->pos;
	size_t cur_page_index = cursor / PAGE_SIZE;
	size_t page_offset = cursor % PAGE_SIZE;
	size_t total_readed = 0;
	size_t available = inode->size - file->pos;

	if(count > available){
		count = available;
	}

	size_t remaining = count;
	while(remaining > 0){
		struct page* page = rino->pages[cur_page_index];
		size_t available_in_page = PAGE_SIZE - page_offset;
		size_t to_read = (remaining < available_in_page) ? remaining : available_in_page;

		if(page){
			char* page_ptr = (char*)page_to_virt(page);
			memcpy(buffer + total_readed, page_ptr + page_offset, to_read);
		}else{
			memset(buffer + total_readed, 0, to_read);
		}

		remaining -= to_read;
		total_readed += to_read;
		page_offset = 0;
		cur_page_index++;
	}

	file->pos += total_readed;
	return total_readed;
}

static int ramfs_write(struct file *file, const void *buffer, uint32_t count){
	struct inode *inode = file->inode;
	struct ramfs_inode *rino = inode->private_data;

	if(count == 0){
		return 0;
	}

	size_t cursor = file->pos;
	size_t cur_page_index = cursor / PAGE_SIZE;
	size_t page_offset = cursor % PAGE_SIZE;
	size_t remaining = count;
	size_t total_written = 0;

	while(remaining > 0){
		if(cur_page_index >= rino->page_capacity){
			size_t new_cap = rino->page_capacity ? rino->page_capacity * 2 : 4;
			int res = ramfs_grow_pages_arr(rino, new_cap);
			if(res) return total_written ? total_written : NO_MEMORY;;
		}

		struct page *page = rino->pages[cur_page_index];
		if(!page){
			page = page_alloc(0,PG_KERNEL);
			if(!page){
				return total_written ? total_written : NO_MEMORY;
			}

			memset((void*)page_to_virt(page), 0x0, PAGE_SIZE);
			rino->pages[cur_page_index] = page;
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

	file->pos += total_written;
	if(file->pos > inode->size){
		inode->size = file->pos;
	}

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
			return INVALID_ARG;
	}

	if(target < 0){
		return INVALID_ARG;
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