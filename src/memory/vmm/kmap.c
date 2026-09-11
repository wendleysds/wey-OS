#include <sync/spinlock.h>
#include <kernel/init.h>
#include <mm/page.h>
#include <mm/mmu.h>

#include <def/config.h>
#include <def/errno.h>

#define KMAP_SLOT_COUNT \
    (KERNEL_HIGHMEM_SIZE / PAGE_SIZE)

extern struct paging_ctx kernel_ctx;

static struct page *kmap_page_slots[KMAP_SLOT_COUNT];
static spinlock_t kmap_lock;
static size_t kmap_slot_hint;

static inline uintptr_t kmap_slot_to_address(size_t slot) {
    return KERNEL_HIGHMEM_START + (slot * PAGE_SIZE);
}

static inline intptr_t kmap_address_to_slot(uintptr_t addr) {
    if (addr < KERNEL_HIGHMEM_START ||addr >= KERNEL_HIGHMEM_START + KERNEL_HIGHMEM_SIZE)
        return -1;

    return (intptr_t)((addr - KERNEL_HIGHMEM_START) / PAGE_SIZE);
}

static intptr_t kmap_find_free_slot(void) {
    for (size_t i = 0; i < KMAP_SLOT_COUNT; i++) {
        size_t idx = (kmap_slot_hint + i) % KMAP_SLOT_COUNT;
        if (kmap_page_slots[idx] == NULL) {
            kmap_slot_hint = (idx + 1) % KMAP_SLOT_COUNT;
            return (intptr_t)idx;
        }
    }

    return -1;
}

static intptr_t kmap_find_page(struct page *page) {
    uintptr_t vaddr = (uintptr_t)page->private;
    if(vaddr != 0){
        intptr_t slot = kmap_address_to_slot(vaddr);
        if(slot >= 0 && kmap_page_slots[slot] == page){
            return slot;
        }
    }

    return -1;
}

void *kmap(struct page *page) {
    if (!page)
        return NULL;

    if (!(page->flags & PG_HIGHMEM))
        return (void *)page_to_virt(page);

    spin_lock(&kmap_lock);

    if (kmap_find_page(page) >= 0) {
        spin_unlock(&kmap_lock);
        return NULL;
    }

    intptr_t slot = kmap_find_free_slot();

    if (slot < 0) {
        spin_unlock(&kmap_lock);
        return NULL;
    }

    uintptr_t vaddr =
        kmap_slot_to_address((size_t)slot);

    int res = mmu_mmap(
        &kernel_ctx,
        page_to_phys(page),
        vaddr,
        PAGE_SIZE,
        MEM_READ | MEM_WRITE
    );

    if (res < 0) {
        spin_unlock(&kmap_lock);
        return NULL;
    }

    kmap_page_slots[slot] = page;
    page->private = (void*)vaddr;

    spin_unlock(&kmap_lock);

    return (void *)vaddr;
}

void kunmap(struct page *page) {
    if (!page || !(page->flags & PG_HIGHMEM))
        return;

    spin_lock(&kmap_lock);

    intptr_t slot = kmap_find_page(page);

    if (slot < 0) {
        spin_unlock(&kmap_lock);
        return;
    }

    uintptr_t vaddr =
        kmap_slot_to_address((size_t)slot);

    mmu_munmap(
        &kernel_ctx,
        vaddr,
        PAGE_SIZE
    );

    kmap_page_slots[slot] = NULL;

    spin_unlock(&kmap_lock);
}

static __init int kmap_init(void) {
    spinlock_init(&kmap_lock);
    return SUCCESS;
}

core_initcall(kmap_init);