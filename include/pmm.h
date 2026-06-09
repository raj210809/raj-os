/*
 * pmm.h — Physical Memory Manager (4 KiB page frames, bitmap).
 *
 * Each bit in the bitmap represents one 4 KiB frame: 1 = used, 0 = free.
 * Usable RAM comes from the E820 map (type 1); reserved areas are pre-marked.
 */

#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PAGE_SIZE       4096U
#define PAGE_SHIFT      12

/*
 * Bitmap: 1 bit per 4 KiB frame starting at physical 1 MiB.
 * 32768 pages = 128 MiB; stored in kernel BSS (not fixed low RAM like 0x7000).
 */
#define PMM_MAX_PAGES     32768U
#define PMM_BITMAP_BYTES  (PMM_MAX_PAGES / 8U)

void pmm_init(void);

/* Allocate one free 4 KiB page frame; returns physical address or 0 on failure. */
uint32_t pmm_alloc_page(void);

/* Mark one page frame free again. */
void pmm_free_page(uint32_t phys);

/* Print total / used / free memory on VGA + serial. */
void pmm_print_stats(void);

uint32_t pmm_total_pages(void);
uint32_t pmm_free_pages(void);

#endif /* PMM_H */
