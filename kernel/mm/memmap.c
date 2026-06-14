/*
 * memmap.c — Read and display the E820 map left at MEMMAP_PHYS by the bootloader.
 */

#include "memmap.h"
#include "serial.h"
#include "vga.h"

static const uint8_t *mmap_raw(void)
{
    return (const uint8_t *)MEMMAP_PHYS;
}

static const e820_entry_t *mmap_entry(uint32_t index)
{
    return (const e820_entry_t *)(mmap_raw() + 4 + index * sizeof(e820_entry_t));
}

static const char *e820_type_name(uint32_t type)
{
    switch (type) {
    case E820_TYPE_USABLE:       return "usable";
    case E820_TYPE_RESERVED:     return "reserved";
    case E820_TYPE_ACPI_RECLAIM: return "ACPI reclaim";
    default:                     return "other";
    }
}

uint64_t memmap_print(void)
{
    uint32_t count = *(const uint32_t *)MEMMAP_PHYS;
    uint64_t usable = 0;

    serial_puts("[memmap] E820 entries: ");
    serial_print_hex32(count);
    serial_putln("");

    for (uint32_t i = 0; i < count; i++) {
        const e820_entry_t *e = mmap_entry(i);

        serial_puts("  [");
        serial_print_hex32(i);
        serial_puts("] base=");
        serial_print_hex64(e->base);
        serial_puts(" len=");
        serial_print_hex64(e->length);
        serial_puts(" type=");
        serial_print_hex32(e->type);
        serial_puts(" (");
        serial_puts(e820_type_name(e->type));
        serial_puts(")\n");

        if (e->type == E820_TYPE_USABLE) {
            usable += e->length;
        }
    }

    serial_puts("[memmap] total usable RAM = ");
    serial_print_hex64(usable);
    serial_puts(" bytes\n");

    vga_write_at(4, 0, "--- E820 / PMM ---");
    vga_puts("Usable RAM (type 1): ");
    {
        uint32_t mb = (uint32_t)(usable / (1024U * 1024U));
        char buf[12];
        int i = 10;
        buf[11] = '\0';
        if (mb == 0) {
            vga_puts("<1");
        } else {
            while (mb > 0 && i >= 0) {
                buf[i--] = (char)('0' + (mb % 10U));
                mb /= 10U;
            }
            vga_puts(&buf[i + 1]);
        }
    }
    vga_puts(" MiB (approx)\n\n");

    return usable;
}
