/*
 * kmalloc.h — Kernel heap (linked-list free blocks on top of PMM 4 KiB pages).
 */

#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>

void kmalloc_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void kmalloc_print_stats(void);

#endif /* KMALLOC_H */
