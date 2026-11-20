#include <fs/vfs.h>
#include <mm/page.h>
#include <mm/kmap.h>
#include <mm/pagecache.h>
#include <def/errno.h>
#include <def/config.h>
#include <lib/string.h>
#include <stddef.h>

#include "ramfs_internal.h"

/* ramfs doesn't have backing store: new page = zeros */
static int ramfs_readpage(struct address_space *m, struct page *page)
{
	pagecache_zero(page, 0, PAGE_SIZE);
	return 0;
}

const struct address_space_operations ramfs_aops = {
	.readpage = ramfs_readpage,
	.writepage = NULL,
};

static ssize_t ramfs_read(struct file *file, void *buffer, size_t count, off_t *offset)
{
	struct inode *inode = file->inode;
	struct address_space *m = inode->i_mapping;

	if (*offset < 0 || (size_t)*offset >= inode->size)
		return 0;

	size_t cursor = (size_t)*offset;
	size_t avail = inode->size - cursor;
	if (count > avail)
		count = avail;

	size_t done = 0;
	while (done < count) {
		size_t poff = cursor % PAGE_SIZE;
		size_t n = PAGE_SIZE - poff;
		if (n > count - done)
			n = count - done;

		/* create=false: holes (never written pages) read as zero without using RAM */
		struct page *page = pagecache_get_page(m, cursor / PAGE_SIZE, false);
		if (page) {
			void *v = kmap(page);
			if (!v) {
				page_put(page);
				if (done) break;
				return -ENOMEM;
			}
			memcpy((char *)buffer + done, (char *)v + poff, n);
			kunmap(page);
			page_put(page);
		} else {
			memset((char *)buffer + done, 0, n);
		}

		done += n;
		cursor += n;
	}

	*offset = (off_t)cursor;
	return (ssize_t)done;
}

static ssize_t ramfs_write(struct file *file, const void *buffer, size_t count, off_t *offset)
{
	struct inode *inode = file->inode;
	struct address_space *m = inode->i_mapping;

	if (count == 0)
		return 0;
	if (*offset < 0)
		return -EINVAL;

	size_t cursor = (size_t)*offset;
	if (cursor + count < cursor)
		return -EFBIG;

	size_t done = 0;
	while (done < count) {
		size_t poff = cursor % PAGE_SIZE;
		size_t n = PAGE_SIZE - poff;
		if (n > count - done)
			n = count - done;

		struct page *page = pagecache_get_page(m, cursor / PAGE_SIZE, true);
		if (IS_ERR(page)) {
			if (done) break;
			return PTR_ERR(page);
		}

		void *v = kmap(page);
		if (!v) {
			page_put(page);
			if (done) break;
			return -ENOMEM;
		}

		memcpy((char *)v + poff, (const char *)buffer + done, n);
		kunmap(page);
		set_page_dirty(page);
		page_put(page);

		done += n;
		cursor += n;
	}

	spin_lock(&inode->lock);
	if (cursor > inode->size)
		inode->size = cursor;
	spin_unlock(&inode->lock);

	*offset = (off_t)cursor;
	return (ssize_t)done;
}

off_t ramfs_lseek(struct file *file, off_t offset, int whence)
{
	size_t filesize = file->inode->size;

	off_t target;
	switch (whence) {
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

	if (target < 0)
		return -EINVAL;

	if (target > (off_t)filesize) {
		int res = ramfs_do_truncate(file->inode, target);
		if (IS_ERR_VALUE(res))
			return res;
	}

	file->pos = (size_t)target;
	return target;
}

const struct file_operations ramfs_fops = {
	.read = ramfs_read,
	.write = ramfs_write,
	.lseek = ramfs_lseek,
};