/*
 * memmap.h — E820 memory map left by the bootloader at MEMMAP_PHYS.
 *
 * The BIOS INT 15h AX=E820h call can only run in real mode.  The bootloader
 * fills boot_memmap_t; the kernel reads it in protected mode.
 */

#ifndef MEMMAP_H
#define MEMMAP_H

#include <stdint.h>

#define MEMMAP_PHYS         0x8000
#define MEMMAP_MAX_ENTRIES  20

/* ACPI E820 region types (we care most about type 1 = usable RAM). */
#define E820_TYPE_USABLE        1
#define E820_TYPE_RESERVED      2
#define E820_TYPE_ACPI_RECLAIM  3

/* One 24-byte record from the BIOS (same layout as boot/e820.inc). */
typedef struct e820_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) e820_entry_t;

typedef struct boot_memmap {
    uint32_t      count;
    e820_entry_t  entries[MEMMAP_MAX_ENTRIES];
} __attribute__((packed)) boot_memmap_t;

#define boot_memmap ((const boot_memmap_t *)MEMMAP_PHYS)

/* Print every region; returns total bytes of type E820_TYPE_USABLE. */
uint64_t memmap_print(void);

#endif /* MEMMAP_H */
