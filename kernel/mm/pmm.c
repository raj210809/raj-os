/*
 * pmm.c — Physical Memory Manager using a bitmap (1 bit per 4 KiB frame).
 *
 * Tracks page frames at/above PMM_FRAME_BASE (1 MiB).  Regions from the E820
 * map drive which frames are free; reserved/non-usable areas are pre-marked.
 */

#include "memmap.h"
#include "pmm.h"
#include "serial.h"
#include "vga.h"

#include <stdint.h>
#include <stddef.h>

#define PMM_FRAME_BASE  0x100000U

/* In kernel BSS — do not place at 0x7000; a 4 KiB map there clobbers low RAM. */
static uint8_t bitmap[PMM_BITMAP_BYTES];

static uint32_t total_pages;
static uint32_t used_pages;

static int bitmap_test(uint32_t page)
{
    if (page >= PMM_MAX_PAGES) {
        return 1;
    }
    return (bitmap[page / 8U] >> (page % 8U)) & 1;
}

static void bitmap_set(uint32_t page)
{
    if (page >= PMM_MAX_PAGES) {
        return;
    }
    bitmap[page / 8U] |= (uint8_t)(1U << (page % 8U));
}

static void bitmap_clear(uint32_t page)
{
    if (page >= PMM_MAX_PAGES) {
        return;
    }
    bitmap[page / 8U] &= (uint8_t)~(1U << (page % 8U));
}

static void bitmap_mark_used(uint32_t page)
{
    if (page >= PMM_MAX_PAGES || bitmap_test(page)) {
        return;
    }
    bitmap_set(page);
    used_pages++;
}

static uint32_t phys_to_page(uint32_t phys)
{
    if (phys < PMM_FRAME_BASE) {
        return 0xFFFFFFFFU;
    }
    uint32_t page = (phys - PMM_FRAME_BASE) / PAGE_SIZE;
    if (page >= PMM_MAX_PAGES) {
        return 0xFFFFFFFFU;
    }
    return page;
}

/* Mark every page frame in [phys, phys+len) that we track (32-bit physical only). */
static void mark_phys_range(uint32_t phys, uint64_t len)
{
    uint64_t end;

    if (len == 0) {
        return;
    }

    end = (uint64_t)phys + len;
    if (end > 0x100000000ULL) {
        end = 0x100000000ULL;
    }

    for (uint64_t addr = phys; addr < end; addr += PAGE_SIZE) {
        uint32_t page = phys_to_page((uint32_t)addr);
        if (page != 0xFFFFFFFFU) {
            bitmap_mark_used(page);
        }
    }
}

/* Reserve one non-usable E820 region without truncating high bases to address 0. */
static void mark_e820_reserved(const e820_entry_t *e)
{
    uint64_t end;

    if (e->type == E820_TYPE_USABLE || e->length == 0) {
        return;
    }

    /* Regions entirely above 4 GiB are invisible to this 32-bit PMM. */
    if (e->base >= 0x100000000ULL) {
        return;
    }

    end = e->base + e->length;
    if (end > 0x100000000ULL) {
        end = 0x100000000ULL;
    }

    mark_phys_range((uint32_t)e->base, end - e->base);
}

void pmm_init(void)
{
    uint32_t count = *(const uint32_t *)MEMMAP_PHYS;

    total_pages = 0;
    used_pages  = 0;

    for (uint32_t i = 0; i < PMM_BITMAP_BYTES; i++) {
        bitmap[i] = 0;
    }

    /* How many frames (>= 1 MiB) exist across usable E820 regions? */
    for (uint32_t i = 0; i < count; i++) {
        const e820_entry_t *e = (const e820_entry_t *)(MEMMAP_PHYS + 4 + i * sizeof(e820_entry_t));
        uint64_t end;
        uint32_t base;

        if (e->type != E820_TYPE_USABLE) {
            continue;
        }

        end = e->base + e->length;
        if (end <= PMM_FRAME_BASE) {
            continue;
        }

        base = (e->base < PMM_FRAME_BASE) ? PMM_FRAME_BASE : (uint32_t)e->base;
        while ((uint64_t)base + PAGE_SIZE <= end && total_pages < PMM_MAX_PAGES) {
            total_pages++;
            base += PAGE_SIZE;
        }
    }

    /*
     * Mark non-usable E820 regions as used (reserved, ACPI, etc.).
     * Also reserve our bitmap buffer if it falls in tracked RAM.
     */
    for (uint32_t i = 0; i < count; i++) {
        const e820_entry_t *e = (const e820_entry_t *)(MEMMAP_PHYS + 4 + i * sizeof(e820_entry_t));
        mark_e820_reserved(e);
    }

    serial_puts("[pmm] bitmap @ ");
    serial_print_hex32((uint32_t)(uintptr_t)&bitmap[0]);
    serial_puts("  pages tracked: ");
    serial_print_hex32(total_pages);
    serial_putln("");
}

uint32_t pmm_alloc_page(void)
{
    for (uint32_t page = 0; page < total_pages; page++) {
        if (!bitmap_test(page)) {
            bitmap_mark_used(page);
            return PMM_FRAME_BASE + page * PAGE_SIZE;
        }
    }
    return 0;
}

void pmm_free_page(uint32_t phys)
{
    uint32_t page = phys_to_page(phys);
    if (page == 0xFFFFFFFFU || page >= total_pages) {
        return;
    }
    if (bitmap_test(page)) {
        bitmap_clear(page);
        if (used_pages > 0) {
            used_pages--;
        }
    }
}

uint32_t pmm_total_pages(void) { return total_pages; }
uint32_t pmm_free_pages(void)
{
    return (used_pages >= total_pages) ? 0U : (total_pages - used_pages);
}

void pmm_print_stats(void)
{
    uint32_t free_p  = pmm_free_pages();
    uint32_t used_p  = used_pages;
    uint32_t free_kb = (free_p * PAGE_SIZE) / 1024U;
    uint32_t used_kb = (used_p * PAGE_SIZE) / 1024U;

    serial_puts("[pmm] total pages=");
    serial_print_hex32(total_pages);
    serial_puts(" used=");
    serial_print_hex32(used_p);
    serial_puts(" free=");
    serial_print_hex32(free_p);
    serial_putln("");

    vga_puts("--- PMM (bitmap) ---\n");
    vga_puts("Page size: 4096 bytes\n");
    vga_puts("Free RAM:  ");
    {
        char buf[12];
        uint32_t v = free_kb;
        int i = 10;
        buf[11] = '\0';
        if (v == 0) {
            vga_puts("0");
        } else {
            while (v > 0 && i >= 0) {
                buf[i--] = (char)('0' + (v % 10U));
                v /= 10U;
            }
            vga_puts(&buf[i + 1]);
        }
    }
    vga_puts(" KiB  Used: ");
    {
        char buf[12];
        uint32_t v = used_kb;
        int i = 10;
        buf[11] = '\0';
        if (v == 0) {
            vga_puts("0");
        } else {
            while (v > 0 && i >= 0) {
                buf[i--] = (char)('0' + (v % 10U));
                v /= 10U;
            }
            vga_puts(&buf[i + 1]);
        }
    }
    vga_puts(" KiB\n\n");
}
