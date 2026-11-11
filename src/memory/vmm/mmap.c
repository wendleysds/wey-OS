#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/uaccess.h>
#include <fs/fdtable.h>
#include <fs/stat.h>
#include <mm/vma.h>
#include <mm/page.h>
#include <mm/kmap.h>
#include <lib/string.h>
#include <def/errno.h>

#include <uapi/sys/mman.h>

#define MMAP_BASE   0x40000000UL
#define MMAP_TOP    ALIGN_DOWN((PROC_USER_STACK_VIRUTAL_TOP - PROC_USER_STACK_MAX_SIZE), PAGE_SIZE)
#define USER_LIMIT  ((vaddr_t)USER_SPACE_END + 1)

#define ALIGN_UP(v, a)   (((v) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(v, a) ((v) & ~((a) - 1))

static inline mem_flags_t prot_to_mem_flags(int prot)
{
	mem_flags_t f = MEM_USER;

	if (prot & (PROT_READ | PROT_WRITE | PROT_EXEC)) f |= MEM_READ;
	if (prot & PROT_WRITE) f |= MEM_WRITE;
	if (prot & PROT_EXEC)  f |= MEM_EXEC;

	return f;
}

static inline prot_flags_t map_to_prot_flags(int flags)
{
	prot_flags_t f = 0;

	if (flags & MAP_PRIVATE)   f |= PROT_MAP_PRIVATE;
	if (flags & MAP_ANONYMOUS) f |= PROT_MAP_ANONYMOUS;
	if (flags & MAP_FIXED)     f |= PROT_MAP_FIXED;
	if (flags & MAP_POPULATE)  f |= PROT_MAP_POPULATE;

	return f;
}

static long mmap_check_args(vaddr_t addr, size_t len, int prot, int flags,
                            off_t off, size_t *alen)
{
	if (!len)
		return -EINVAL;
	if (!!(flags & MAP_SHARED) == !!(flags & MAP_PRIVATE))
		return -EINVAL;
	if (flags & ~MAP_VALID)
		return -EINVAL;
	if (prot & ~PROT_VALID)
		return -EINVAL;
	if (off < 0 || (off & (PAGE_SIZE - 1)))
		return -EINVAL;
	if (len > MMAP_TOP)
		return -ENOMEM;

	*alen = ALIGN_UP(len, PAGE_SIZE);

	if (flags & MAP_FIXED) {
		if (!addr || (addr & (PAGE_SIZE - 1)))
			return -EINVAL;
		if (addr + *alen < addr || addr + *alen > USER_LIMIT)
			return -EINVAL;
	}

	return 0;
}

static long mmap_check_file(struct file *file)
{
	if (!file->inode || !S_ISREG(file->inode->mode))
		return -ENODEV;
	if (!file->f_op || !file->f_op->read)
		return -ENODEV;
	if ((file->flags & O_WRONLY) && !(file->flags & O_RDWR))
		return -EACCES;
	return 0;
}

static bool range_is_free_locked(struct mm_struct *mm, vaddr_t start, vaddr_t end)
{
	for (struct vm_region *r = mm->vma; r; r = r->next) {
		if (r->start >= end)
			break;
		if (r->end > start)
			return false;
	}
	return true;
}

static vaddr_t find_free_area_locked(struct mm_struct *mm, vaddr_t hint, size_t len)
{
	if (hint && !(hint & (PAGE_SIZE - 1)) &&
	    hint >= MMAP_BASE && hint <= MMAP_TOP && len <= MMAP_TOP - hint &&
	    range_is_free_locked(mm, hint, hint + len))
		return hint;

	vaddr_t addr = MMAP_BASE;

	for (struct vm_region *r = mm->vma; r; r = r->next) {
		if (r->end <= addr)
			continue;
		if (r->start >= addr + len)
			break;
		addr = ALIGN_UP(r->end, PAGE_SIZE);
	}

	if (addr > MMAP_TOP || len > MMAP_TOP - addr)
		return 0;

	return addr;
}

static void unmap_and_put_pages(struct mm_struct *mm, vaddr_t addr, size_t size)
{
	if (!size)
		return;

	const vaddr_t start = ALIGN_DOWN(addr, PAGE_SIZE);
	const vaddr_t end   = ALIGN_UP(addr + size, PAGE_SIZE);

	for (vaddr_t va = start; va < end; va += PAGE_SIZE) {
		uintptr_t phys = mmu_translate(mm->ctx, va);
		if (phys) {
			struct page *page = phys_to_page(phys);
			if (page)
				page_put(page);
		}
	}

	mmu_munmap(mm->ctx, start, end - start);
}

static int do_mmap_populate(struct mm_struct *mm, struct vm_region *region)
{
	if (!(region->mem_flags & (MEM_READ | MEM_WRITE | MEM_EXEC)))
		return 0;

	vaddr_t addr = region->start;
	off_t file_offset = region->file_offset;
	int err = 0;

	while (addr < region->end) {
		struct page *page = page_alloc(0, PG_USER);
		if (!page) { err = -ENOMEM; goto rollback; }

		void *kvirt = kmap(page);
		if (!kvirt) { page_free(page); err = -ENOMEM; goto rollback; }

		memset(kvirt, 0, PAGE_SIZE);

		if (region->file) {
			int n = vfs_pread(region->file, kvirt, PAGE_SIZE, file_offset);
			if (n < 0) {
				kunmap(page);
				page_free(page);
				err = n;
				goto rollback;
			}
			file_offset += PAGE_SIZE;
		}

		kunmap(page);

		err = mmu_mmap(mm->ctx, page_to_phys(page), addr, PAGE_SIZE, region->mem_flags);
		if (err < 0) { page_free(page); goto rollback; }

		mmu_invlpg(mm->ctx, addr);
		addr += PAGE_SIZE;
	}

	return 0;

rollback:
	unmap_and_put_pages(mm, region->start, addr - region->start);
	return err;
}

SYSCALL_DEFINE6(mmap,
	__user void*, addr,
	size_t,       len,
	int,          prot,
	int,          flags,
	int,          fd,
	off_t,        offset
){
	struct mm_struct *mm = current->mm;
	struct file *file = NULL;
	struct vm_region *region = NULL;
	struct vm_region *spare  = NULL;
	struct vm_region *dead   = NULL;
	const vaddr_t hint = (vaddr_t)addr;
	vaddr_t map_addr = hint;
	unsigned long irq;
	size_t alen;
	long err;

	err = mmap_check_args(hint, len, prot, flags, offset, &alen);
	if (err)
		return err;

	/* TODO: MAP_SHARED need page cache peer inode (address_space). */
	if (flags & MAP_SHARED)
		return -ENOTSUP;

	if (flags & MAP_ANONYMOUS){
		offset = 0;
	} else {
		file = file_table_get_file(current->files, fd);
		if (!file)
			return -EBADF;

		err = mmap_check_file(file);
		if (err)
			goto out;
	}

	region = vma_alloc(prot_to_mem_flags(prot), map_to_prot_flags(flags), file, offset);
	spare  = kzalloc(sizeof(struct vm_region));
	if (!region || !spare) {
		err = -ENOMEM;
		goto out;
	}

	spin_lock_irqsave(&mm->lock, &irq);

	if (flags & MAP_FIXED) {
		err = __vma_unlink_range(mm, map_addr, map_addr + alen, &spare, &dead);
		if (err) {
			spin_unlock_irqrestore(&mm->lock, &irq);
			goto out;
		}
		unmap_and_put_pages(mm, map_addr, alen);
	} else {
		map_addr = find_free_area_locked(mm, hint, alen);
		if (!map_addr) {
			spin_unlock_irqrestore(&mm->lock, &irq);
			err = -ENOMEM;
			goto out;
		}
	}

	region->start = map_addr;
	region->end   = map_addr + alen;

	err = __vma_insert(mm, region);
	spin_unlock_irqrestore(&mm->lock, &irq);

	if (err)
		goto out;

	struct vm_region *mapped = region;
	region = NULL;

	if (flags & MAP_POPULATE) {
		err = do_mmap_populate(mm, mapped);
		if (err < 0) {
			vma_remove(mm, map_addr);
			goto out;
		}
	}

	err = (long)map_addr;

out:
	__vma_free_list(dead);
	if (spare)  kfree(spare);
	if (region) kfree(region);
	if (file)   file_put(file);
	return err;
}

SYSCALL_DEFINE2(munmap,
	__user void*, addr,
	size_t,       len
){
	struct mm_struct *mm = current->mm;
	const vaddr_t start = (vaddr_t)addr;
	unsigned long irq;
	long err;

	if (!len || (start & (PAGE_SIZE - 1)))
		return -EINVAL;
	if (len > USER_LIMIT)
		return -EINVAL;

	const size_t alen = ALIGN_UP(len, PAGE_SIZE);
	if (start + alen < start || start + alen > USER_LIMIT)
		return -EINVAL;

	struct vm_region *spare = kzalloc(sizeof(struct vm_region));
	struct vm_region *dead  = NULL;
	if (!spare)
		return -ENOMEM;

	spin_lock_irqsave(&mm->lock, &irq);
	err = __vma_unlink_range(mm, start, start + alen, &spare, &dead);
	if (!err)
		unmap_and_put_pages(mm, start, alen);
	spin_unlock_irqrestore(&mm->lock, &irq);

	__vma_free_list(dead);
	if (spare)
		kfree(spare);

	return err;
}