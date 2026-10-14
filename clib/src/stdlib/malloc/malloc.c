#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define BLOCK_SIZE 32
#define HEAP_SIZE  (1024 * 64)
#define NUM_BLOCKS (HEAP_SIZE / BLOCK_SIZE)

#define BLOCK_FREE     0x00
#define BLOCK_USED     0x01
#define BLOCK_HEAD     0x02
#define BLOCK_HAS_NEXT 0x04

static uint8_t heap[HEAP_SIZE] __attribute__((aligned(16)));
static uint8_t block_tags[NUM_BLOCKS];

void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size_t needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (needed > NUM_BLOCKS) {
        return NULL;
    }

    size_t count = 0;
    size_t start_idx = 0;

    for (size_t i = 0; i < NUM_BLOCKS; i++) {
        if ((block_tags[i] & BLOCK_USED) == 0) {
            if (count == 0) {
                start_idx = i;
            }
            count++;
            if (count == needed) {
                for (size_t j = 0; j < needed; j++) {
                    uint8_t flags = BLOCK_USED;
                    if (j == 0) {
                        flags |= BLOCK_HEAD;
                    }
                    if (j < needed - 1) {
                        flags |= BLOCK_HAS_NEXT;
                    }
                    block_tags[start_idx + j] = flags;
                }
                return (void *)&heap[start_idx * BLOCK_SIZE];
            }
        } else {
            count = 0;
        }
    }

    return NULL;
}

void free(void *ptr) {
    if (!ptr) {
        return;
    }

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t heap_start = (uintptr_t)heap;
    uintptr_t heap_end = heap_start + HEAP_SIZE;

    if (addr < heap_start || addr >= heap_end) {
        return;
    }

    if ((addr - heap_start) % BLOCK_SIZE != 0) {
        return;
    }

    size_t idx = (addr - heap_start) / BLOCK_SIZE;
    if (!(block_tags[idx] & BLOCK_HEAD) || !(block_tags[idx] & BLOCK_USED)) {
        return;
    }

    while (idx < NUM_BLOCKS) {
        uint8_t flags = block_tags[idx];
        block_tags[idx] = BLOCK_FREE;
        if (!(flags & BLOCK_HAS_NEXT)) {
            break;
        }
        idx++;
    }
}

void *realloc(void *ptr, size_t nsize) {
    if (!ptr) {
        return malloc(nsize);
    }
    if (nsize == 0) {
        free(ptr);
        return NULL;
    }

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t heap_start = (uintptr_t)heap;
    uintptr_t heap_end = heap_start + HEAP_SIZE;

    if (addr < heap_start || addr >= heap_end) {
        return NULL;
    }
    if ((addr - heap_start) % BLOCK_SIZE != 0) {
        return NULL;
    }

    size_t idx = (addr - heap_start) / BLOCK_SIZE;
    if (!(block_tags[idx] & BLOCK_HEAD) || !(block_tags[idx] & BLOCK_USED)) {
        return NULL;
    }

    size_t cur_blocks = 0;
    size_t curr = idx;
    while (curr < NUM_BLOCKS) {
        cur_blocks++;
        if (!(block_tags[curr] & BLOCK_HAS_NEXT)) {
            break;
        }
        curr++;
    }

    size_t cur_size = cur_blocks * BLOCK_SIZE;
    size_t needed = (nsize + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if (needed == cur_blocks) {
        return ptr;
    }

    if (needed < cur_blocks) {
        for (size_t j = needed; j < cur_blocks; j++) {
            block_tags[idx + j] = BLOCK_FREE;
        }
        block_tags[idx + needed - 1] &= ~BLOCK_HAS_NEXT;
        return ptr;
    }

    // Attempt in-place expansion if following blocks are free
    size_t extra = needed - cur_blocks;
    bool can_expand = true;
    for (size_t j = 0; j < extra; j++) {
        size_t next_idx = idx + cur_blocks + j;
        if (next_idx >= NUM_BLOCKS || (block_tags[next_idx] & BLOCK_USED)) {
            can_expand = false;
            break;
        }
    }

    if (can_expand) {
        block_tags[idx + cur_blocks - 1] |= BLOCK_HAS_NEXT;
        for (size_t j = 0; j < extra; j++) {
            size_t next_idx = idx + cur_blocks + j;
            uint8_t flags = BLOCK_USED;
            if (j < extra - 1) {
                flags |= BLOCK_HAS_NEXT;
            }
            block_tags[next_idx] = flags;
        }
        return ptr;
    }

    void *new_ptr = malloc(nsize);
    if (!new_ptr) {
        return NULL;
    }

    size_t copy_size = (cur_size < nsize) ? cur_size : nsize;
    memcpy(new_ptr, ptr, copy_size);
    free(ptr);
    return new_ptr;
}

void *calloc(size_t nmemb, size_t size) {
    if (nmemb != 0 && size > (size_t)-1 / nmemb) {
        return NULL;
    }
    size_t total = nmemb * size;
    void *p = malloc(total);
    if (p) {
        memset(p, 0, total);
    }
    return p;
}
