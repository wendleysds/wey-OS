#include <mm/pagecache.h>
#include <mm/vma.h>
#include <mm/mmu.h>
#include <mm/kmap.h>
#include <mm/kheap.h>
#include <kernel/sched.h>
#include <kernel/wait.h>
#include <kernel/init.h>
#include <kernel/printk.h>
#include <lib/string.h>
#include <def/errno.h>
#include <def/config.h>

static struct wait_queue_head page_waitq;

static int __init pagecache_init(void)
{
	wait_queue_head_init(&page_waitq);
	return SUCCESS;
}

core_initcall(pagecache_init);

static void unlock_page(struct page *page)
{
	page->flags &= ~PG_LOCKED;
	wake_up_all(&page_waitq);
}

static void wait_on_page_locked(struct page *page)
{
	struct wait_queue_entry w;

	wait_queue_entry_init(&w, current, task_default_wakeup);
	wait_queue_add(&page_waitq, &w);

	while (page->flags & PG_LOCKED)
		sleep_uninterruptible();

	wait_queue_remove(&page_waitq, &w);
}

void address_space_init(struct address_space *m, struct inode *host,
                        const struct address_space_operations *ops)
{
	m->host = host;
	m->a_ops = ops;

	spinlock_init(&m->lock);
	m->pages = NULL;
	m->capacity = 0;
	m->nrpages = 0;

	spinlock_init(&m->i_mmap_lock);
	INIT_LIST_HEAD(&m->i_mmap);
}

void pagecache_zero(struct page *page, size_t from, size_t to)
{
	void *v = kmap(page);
	if (!v)
		return;

	memset((char *)v + from, 0, to - from);
	kunmap(page);
}

/* address_space mm->lock holded */
static int mapping_grow(struct address_space *m, size_t need)
{
	size_t cap = m->capacity ? m->capacity : 16;
	while (cap < need)
		cap *= 2;

	struct page **np = kcalloc(cap, sizeof(*np));
	if (!np)
		return -ENOMEM;

	if (m->pages) {
		memcpy(np, m->pages, m->capacity * sizeof(*np));
		kfree(m->pages);
	}

	m->pages = np;
	m->capacity = cap;
	return 0;
}

struct page *pagecache_lookup(struct address_space *m, pgoff_t idx)
{
	struct page *page = NULL;

	spin_lock(&m->lock);
	if (idx < m->capacity && (page = m->pages[idx]))
		page_get(page);
	spin_unlock(&m->lock);

	return page;
}

struct page *pagecache_get_page(struct address_space *m, pgoff_t idx, bool create)
{
	for (;;) {
		struct page *page, *new;
		int err;

		spin_lock(&m->lock);
		page = (idx < m->capacity) ? m->pages[idx] : NULL;
		if (page) {
			page_get(page);
			spin_unlock(&m->lock);

			if (page->flags & PG_LOCKED)
				wait_on_page_locked(page);

			if (page->mapping != m) {
				page_put(page);
				continue;
			}

			return page;
		}
		spin_unlock(&m->lock);

		if (!create)
			return NULL;

		if (!m->a_ops || !m->a_ops->readpage)
			return ERR_PTR(-ENODEV);

		new = page_alloc(0, PG_USER);
		if (!new)
			return ERR_PTR(-ENOMEM);

		new->mapping = m;
		new->index = idx;
		new->flags |= PG_LOCKED;

		spin_lock(&m->lock);

		if (idx >= m->capacity && (err = mapping_grow(m, idx + 1)) != 0) {
			spin_unlock(&m->lock);
			new->mapping = NULL;
			new->flags &= ~PG_LOCKED;
			page_put(new);
			return ERR_PTR(err);
		}

		if (m->pages[idx]) {
			spin_unlock(&m->lock);
			new->mapping = NULL;
			new->flags &= ~PG_LOCKED;
			page_put(new);
			continue;
		}

		m->pages[idx] = new;
		m->nrpages++;
		page_get(new);
		spin_unlock(&m->lock);

		err = m->a_ops->readpage(m, new);
		if (err) {
			spin_lock(&m->lock);
			if (idx < m->capacity && m->pages[idx] == new) {
				m->pages[idx] = NULL;
				m->nrpages--;
			}
			spin_unlock(&m->lock);

			new->mapping = NULL;
			unlock_page(new);
			page_put(new); // cache ref
			page_put(new); // our ref
			return ERR_PTR(err);
		}

		new->flags |= PG_UPTODATE;
		unlock_page(new);
		return new;
	}
}

void i_mmap_add(struct address_space *m, struct vm_region *r)
{
	unsigned long f;

	spin_lock_irqsave(&m->i_mmap_lock, &f);
	list_add_tail(&r->shared, &m->i_mmap);
	spin_unlock_irqrestore(&m->i_mmap_lock, &f);
}

void i_mmap_remove(struct address_space *m, struct vm_region *r)
{
	unsigned long f;

	spin_lock_irqsave(&m->i_mmap_lock, &f);
	list_remove(&r->shared);
	spin_unlock_irqrestore(&m->i_mmap_lock, &f);
}

static uintptr_t region_va(const struct vm_region *r, pgoff_t idx)
{
	off_t off = (off_t)idx * PAGE_SIZE;
	if (off < r->file_offset)
		return 0;

	size_t d = (size_t)(off - r->file_offset);
	if (d >= r->end - r->start)
		return 0;

	return r->start + d;
}

void unmap_mapping_range(struct address_space *m, pgoff_t first, pgoff_t end)
{
	unsigned long f;
	struct vm_region *r;

	spin_lock_irqsave(&m->i_mmap_lock, &f);

	list_for_each_entry(r, &m->i_mmap, shared) {
		struct paging_ctx *ctx = r->mm->ctx;
		pgoff_t lo = (pgoff_t)(r->file_offset / PAGE_SIZE);
		pgoff_t hi = lo + (r->end - r->start) / PAGE_SIZE;

		if (lo < first) lo = first;
		if (hi > end)   hi = end;

		for (pgoff_t i = lo; i < hi; i++) {
			uintptr_t va = region_va(r, i);
			if (!va)
				continue;

			uintptr_t phys = mmu_translate(ctx, va);
			if (!phys)
				continue;

			page_put(phys_to_page(phys)); // PTE ref
			mmu_munmap(ctx, va, PAGE_SIZE);
		}
	}

	spin_unlock_irqrestore(&m->i_mmap_lock, &f);
}

static void mapping_wrprotect_page(struct address_space *m, struct page *page)
{
	unsigned long f;
	struct vm_region *r;

	spin_lock_irqsave(&m->i_mmap_lock, &f);

	list_for_each_entry(r, &m->i_mmap, shared) {
		if (!(r->mem_flags & MEM_WRITE))
			continue;

		uintptr_t va = region_va(r, page->index);
		if (!va)
			continue;

		struct paging_ctx *ctx = r->mm->ctx;
		if (!mmu_present(ctx, va))
			continue;
		if (!(mmu_get_flags(ctx, va) & MEM_WRITE))
			continue;

		mmu_set_flags(ctx, va, r->mem_flags & ~MEM_WRITE);
		mmu_invlpg(ctx, va);
	}

	spin_unlock_irqrestore(&m->i_mmap_lock, &f);
}

void truncate_pagecache(struct address_space *m, off_t new_size)
{
	const pgoff_t first = ((size_t)new_size + PAGE_SIZE - 1) / PAGE_SIZE;
	const size_t tail = (size_t)new_size % PAGE_SIZE;

	unmap_mapping_range(m, first, (pgoff_t)~0UL);

	spin_lock(&m->lock);
	for (pgoff_t i = first; i < m->capacity; i++) {
		struct page *page = m->pages[i];
		if (!page)
			continue;

		m->pages[i] = NULL;
		m->nrpages--;
		page->mapping = NULL;
		page->flags &= ~PG_DIRTY;
		page_put(page);
	}
	spin_unlock(&m->lock);

	// the rest beyond EOF must be zero
	if (tail) {
		struct page *p = pagecache_lookup(m, first - 1);
		if (p) {
			pagecache_zero(p, tail, PAGE_SIZE);
			page_put(p);
		}
	}
}

void pagecache_destroy(struct address_space *m)
{
	if (!list_empty(&m->i_mmap))
		printk("pagecache: inode destruido com VMAs ainda ligadas!\n");

	spin_lock(&m->lock);
	for (size_t i = 0; i < m->capacity; i++) {
		struct page *page = m->pages[i];
		if (!page)
			continue;

		page->mapping = NULL;
		page->flags &= ~PG_DIRTY;
		page_put(page);
	}

	if (m->pages)
		kfree(m->pages);

	m->pages = NULL;
	m->capacity = 0;
	m->nrpages = 0;
	spin_unlock(&m->lock);
}

int pagecache_writeback(struct address_space *m)
{
	int ret = 0;

	if (!m->a_ops || !m->a_ops->writepage)
		return 0; // ramfs/tmpfs

	for (pgoff_t i = 0; ; i++) {
		struct page *page;

		spin_lock(&m->lock);
		if (i >= m->capacity) {
			spin_unlock(&m->lock);
			break;
		}

		page = m->pages[i];
		if (page && (page->flags & PG_DIRTY))
			page_get(page);
		else
			page = NULL;
		spin_unlock(&m->lock);

		if (!page)
			continue;

		mapping_wrprotect_page(m, page);
		page->flags &= ~PG_DIRTY;

		int err = m->a_ops->writepage(m, page);
		if (err) {
			page->flags |= PG_DIRTY;
			ret = err;
		}

		page_put(page);
	}

	return ret;
}