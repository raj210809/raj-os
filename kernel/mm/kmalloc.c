/*
 * kmalloc.c — First-fit heap with a singly linked free list.
 *
 * Each PMM page is carved into variable-size blocks.  A heap_block header sits
 * immediately before the pointer returned to callers.  Free blocks are kept on a
 * list sorted by address so adjacent free chunks can be merged on kfree().
 */

#include "kmalloc.h"
#include "pmm.h"
#include "serial.h"

#include <stddef.h>
#include <stdint.h>

#define HEAP_MAGIC       0x48454150U  /* "HEAP" */
#define HEAP_ALIGN       16U
#define MIN_BLOCK_SIZE   64U

typedef struct heap_block {
    size_t             total_size;
    uint32_t           magic;
    int                used;
    struct heap_block *next_free;
} heap_block_t;

static heap_block_t *free_list;
static size_t        heap_bytes_total;
static size_t        heap_bytes_used;
static uint32_t      heap_alloc_count;
static uint32_t      heap_free_count;

static size_t align_up(size_t value, size_t align)
{
    return (value + align - 1U) & ~(align - 1U);
}

static size_t block_overhead(void)
{
    return align_up(sizeof(heap_block_t), HEAP_ALIGN);
}

static size_t bytes_needed(size_t payload)
{
    size_t needed = block_overhead() + align_up(payload, HEAP_ALIGN);
    if (needed < MIN_BLOCK_SIZE) {
        needed = MIN_BLOCK_SIZE;
    }
    return needed;
}

static heap_block_t *block_from_ptr(void *ptr)
{
    return (heap_block_t *)((uint8_t *)ptr - block_overhead());
}

static void *ptr_from_block(heap_block_t *block)
{
    return (uint8_t *)block + block_overhead();
}

static void coalesce_free_list(void)
{
    heap_block_t *cur = free_list;

    while (cur != NULL) {
        if (cur->next_free != NULL &&
            (uintptr_t)cur + cur->total_size == (uintptr_t)cur->next_free) {
            cur->total_size += cur->next_free->total_size;
            cur->next_free = cur->next_free->next_free;
        } else {
            cur = cur->next_free;
        }
    }
}

static void free_list_insert(heap_block_t *block)
{
    block->used = 0;
    block->magic = HEAP_MAGIC;
    block->next_free = NULL;

    if (free_list == NULL || (uintptr_t)block < (uintptr_t)free_list) {
        block->next_free = free_list;
        free_list = block;
    } else {
        heap_block_t *cur = free_list;
        while (cur->next_free != NULL && (uintptr_t)cur->next_free < (uintptr_t)block) {
            cur = cur->next_free;
        }
        block->next_free = cur->next_free;
        cur->next_free = block;
    }

    coalesce_free_list();
}

static void free_list_remove(heap_block_t *block)
{
    heap_block_t **link = &free_list;

    while (*link != NULL) {
        if (*link == block) {
            *link = block->next_free;
            block->next_free = NULL;
            return;
        }
        link = &(*link)->next_free;
    }
}

static void split_block(heap_block_t *block, size_t needed)
{
    size_t remainder = block->total_size - needed;

    if (remainder < MIN_BLOCK_SIZE) {
        return;
    }

    block->total_size = needed;

    heap_block_t *tail = (heap_block_t *)((uint8_t *)block + needed);
    tail->total_size = remainder;
    tail->magic = HEAP_MAGIC;
    tail->used = 0;
    tail->next_free = NULL;
    free_list_insert(tail);
}

static int extend_heap(void)
{
    uint32_t phys = pmm_alloc_page();
    if (phys == 0U) {
        return 0;
    }

    heap_block_t *block = (heap_block_t *)(uintptr_t)phys;
    block->total_size = PAGE_SIZE;
    block->magic = HEAP_MAGIC;
    block->used = 0;
    block->next_free = NULL;

    heap_bytes_total += PAGE_SIZE;
    free_list_insert(block);
    return 1;
}

void kmalloc_init(void)
{
    free_list = NULL;
    heap_bytes_total = 0;
    heap_bytes_used = 0;
    heap_alloc_count = 0;
    heap_free_count = 0;

    serial_putln("[kmalloc] linked-list heap ready (grows via pmm_alloc_page)");
}

void *kmalloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    size_t needed = bytes_needed(size);

    for (;;) {
        heap_block_t *cur = free_list;
        while (cur != NULL) {
            if (cur->total_size >= needed) {
                free_list_remove(cur);
                split_block(cur, needed);
                cur->used = 1;
                cur->magic = HEAP_MAGIC;
                cur->next_free = NULL;
                heap_bytes_used += cur->total_size;
                heap_alloc_count++;
                return ptr_from_block(cur);
            }
            cur = cur->next_free;
        }

        if (!extend_heap()) {
            serial_putln("[kmalloc] out of memory");
            return NULL;
        }
    }
}

void kfree(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    heap_block_t *block = block_from_ptr(ptr);

    if (block->magic != HEAP_MAGIC || block->used == 0) {
        serial_putln("[kmalloc] kfree: invalid or double-free");
        return;
    }

    block->used = 0;
    heap_bytes_used -= block->total_size;
    heap_free_count++;
    free_list_insert(block);
}

void kmalloc_print_stats(void)
{
    serial_puts("[kmalloc] heap pages bytes=");
    serial_print_hex64((uint64_t)heap_bytes_total);
    serial_puts(" used=");
    serial_print_hex64((uint64_t)heap_bytes_used);
    serial_puts(" free=");
    serial_print_hex64((uint64_t)(heap_bytes_total - heap_bytes_used));
    serial_puts(" allocs=");
    serial_print_hex32(heap_alloc_count);
    serial_puts(" frees=");
    serial_print_hex32(heap_free_count);
    serial_putln("");
}
