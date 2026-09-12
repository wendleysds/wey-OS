#include <mm/iomem.h>
#include <mm/kheap.h>
#include <mm/mmu.h>
#include <kernel/init.h>
#include <kernel/printk.h>
#include <sync/spinlock.h>
#include <def/config.h>
#include <def/errno.h>
#include <def/bits.h>
#include <lib/list.h>
#include <stdbool.h>

#define ALIGN_UP(v, a)   (((v) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

enum io_block_flags {
	IO_BLOCK_FREE = 0,
	IO_BLOCK_USED = BIT(0),
};

#define IO_BLOCK_SIZE  (PAGE_SIZE * 4)
#define IO_BLOCK_COUNT (KERNEL_NONLINEAR_SIZE / IO_BLOCK_SIZE)

struct io_block {
	uint8_t  flags;
	uint16_t count;
};

static struct io_block block_map[IO_BLOCK_COUNT];
static size_t block_slot_hint = 0;

typedef struct io_mapping {
	paddr_t phys;
	vaddr_t virt;
	size_t size;
	atomic_t refcount;
	struct list_head node;
} io_mapping_t;

extern struct paging_ctx kernel_ctx;
static spinlock_t io_mapping_lock;
static LIST_HEAD(io_mappings_list);

extern int arch_is_system_ram(paddr_t addr, size_t size);

static inline bool block_is_free(size_t idx) {
	return block_map[idx].flags == IO_BLOCK_FREE;
}

static inline bool block_is_used(size_t idx) {
	return block_map[idx].flags & IO_BLOCK_USED;
}

static inline vaddr_t block_to_addr(size_t idx) {
	return KERNEL_NONLINEAR_START + (idx * IO_BLOCK_SIZE);
}

static inline size_t block_to_index(vaddr_t addr) {
	return (addr - KERNEL_NONLINEAR_START) / IO_BLOCK_SIZE;
}

static inline bool block_address_valid(vaddr_t addr) {
	return addr >= KERNEL_NONLINEAR_START && addr < KERNEL_NONLINEAR_END &&
		((addr - KERNEL_NONLINEAR_START) % IO_BLOCK_SIZE) == 0;
}

static inline bool align_up_size(size_t value,
								size_t align,
								size_t *result)
{
	if (value > SIZE_MAX - (align - 1))
		return false;

	*result = ALIGN_UP(value, align);
	return true;
}

/*
* Only the first block of an allocation contains the allocation count.
* block_free() must receive the allocation's base address.
*/
static vaddr_t block_alloc(size_t count) {
	if (count == 0 || count > IO_BLOCK_COUNT)
		return 0;

	size_t found = 0;
	size_t start = 0;

	for (size_t n = 0; n < IO_BLOCK_COUNT; n++) {
		size_t idx = (block_slot_hint + n) % IO_BLOCK_COUNT;

		if (n > 0 && idx == 0) found = 0;

		if (block_is_free(idx)) {

			if (found == 0)
				start = idx;

			found++;

			if (found == count)
				goto found;
		} else {
			found = 0;
		}
	}

	return 0;

found:

	for (size_t i = 0; i < count; i++) {
		block_map[start + i].flags = IO_BLOCK_USED;
		block_map[start + i].count = 0;
	}

	block_map[start].count = count;
	block_slot_hint = (start + count) % IO_BLOCK_COUNT;

	return block_to_addr(start);
}

static void block_free(vaddr_t addr) {
	if (!block_address_valid(addr))
		return;

	size_t idx = block_to_index(addr);

	if (!block_is_used(idx))
		return;

	size_t count = block_map[idx].count;

	if (count == 0 || idx + count > IO_BLOCK_COUNT)
		return;

	for (size_t i = 0; i < count; i++) {

		if (!block_is_used(idx + i))
			return;
	}

	for (size_t i = 0; i < count; i++) {
		block_map[idx + i].flags = IO_BLOCK_FREE;
		block_map[idx + i].count = 0;
	}

	block_slot_hint = idx;
}

void __iomem *ioremap(paddr_t phys, size_t size) {
	if (size == 0)
		return NULL;

	paddr_t phys_base = ALIGN_DOWN(phys, PAGE_SIZE);
	size_t offset = phys - phys_base;

	size_t map_size;
	if (!align_up_size(offset + size, PAGE_SIZE, &map_size))
		return NULL;

	size_t aligned_size;
	if (!align_up_size(map_size, IO_BLOCK_SIZE, &aligned_size))
		return NULL;

	size_t block_count = aligned_size / IO_BLOCK_SIZE;

	unsigned long irqflags;

	spin_lock_irqsave(&io_mapping_lock, &irqflags);

	io_mapping_t *mapping;

	list_for_each_entry(mapping, &io_mappings_list, node) {
		if (phys >= mapping->phys &&
			size <= mapping->size &&
			(phys - mapping->phys) <= mapping->size - size) {

			atomic_inc(&mapping->refcount);

			vaddr_t virt = mapping->virt + (phys - mapping->phys);

			spin_unlock_irqrestore(
				&io_mapping_lock,
				&irqflags
			);

			return (void __iomem *)virt;
		}
	}

	vaddr_t virt = block_alloc(block_count);

	if (!virt) {
		spin_unlock_irqrestore(
			&io_mapping_lock,
			&irqflags
		);

		return NULL;
	}

	io_mapping_t *new_mapping = kzalloc(sizeof(*new_mapping));

	if (!new_mapping) {
		block_free(virt);

		spin_unlock_irqrestore(
			&io_mapping_lock,
			&irqflags
		);

		return NULL;
	}

	int res = mmu_mmap(
		&kernel_ctx,
		phys_base,
		virt,
		map_size,
		MEM_READ |
		MEM_WRITE |
		MEM_DEVICE
	);

	if (res < 0) {
		kfree(new_mapping);
		block_free(virt);

		spin_unlock_irqrestore(
			&io_mapping_lock,
			&irqflags
		);

		return NULL;
	}

	new_mapping->phys = phys_base;
	new_mapping->virt = virt;
	new_mapping->size = map_size;
	atomic_set(&new_mapping->refcount, 1);

	list_add_tail(
		&new_mapping->node,
		&io_mappings_list
	);

	spin_unlock_irqrestore(
		&io_mapping_lock,
		&irqflags
	);

	return (void __iomem *)(virt + offset);
}

/*
* iounmap() accepts any address returned by ioremap(),
* including an address with a physical offset.
*/
void iounmap(void __iomem *addr) {
	if (!addr)
		return;

	vaddr_t virt_addr = (vaddr_t)addr;

	unsigned long irqflags;

	spin_lock_irqsave(
		&io_mapping_lock,
		&irqflags
	);

	io_mapping_t *mapping;
	io_mapping_t *tmp;

	list_for_each_entry_safe(
		mapping,
		tmp,
		&io_mappings_list,
		node
	) {

		if (virt_addr < mapping->virt ||
			virt_addr >= mapping->virt + mapping->size)
			continue;

		if (atomic_dec_and_test(&mapping->refcount)) {
			mmu_munmap(
				&kernel_ctx,
				mapping->virt,
				mapping->size
			);

			list_remove(&mapping->node);

			block_free(mapping->virt);

			kfree(mapping);

			break;
		}
	}

	spin_unlock_irqrestore(
		&io_mapping_lock,
		&irqflags
	);
}

static __init int iomem_init(void) {
	spinlock_init(&io_mapping_lock);
	return SUCCESS;
}

core_initcall(iomem_init);