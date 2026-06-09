/*
 * main.c — C kernel entry (kmain).
 *
 * Flow:
 *   1. Bootloader loads kernel at 0x10000, protected mode.
 *   2. VGA + IDT (CPU exceptions) + IRQ layer (PIC + keyboard).
 *   3. Type in the QEMU window — characters echo via IRQ1 handler.
 */

#include "idt.h"
#include "irq.h"
#include "memmap.h"
#include "pmm.h"
#include "serial.h"
#include "vga.h"

static void kernel_banner(void)
{
    vga_puts("=== C Kernel (32-bit Protected Mode) ===\n\n");
    vga_puts("VGA:      text driver @ 0xB8000\n");
    vga_puts("IDT:      vectors 0-31  CPU exceptions\n");
    vga_puts("IRQ/PIC:  vectors 32-47 hardware (keyboard on IRQ1)\n");
    vga_puts("PMM:      bitmap in kernel BSS (tracks 128 MiB)\n");
    vga_puts("\n");
    vga_write_at(6, 0, "Mode: protected (CR0.PE=1, flat segments)");
    vga_write_at(8, 0, "A20:  enabled");
    vga_write_at(10, 0, "GDT:  code 0x08 + data 0x10");
}

void kmain(void)
{
    serial_putln("[kmain] entered C kernel");

    vga_init();
    serial_putln("[kmain] VGA ready");

    kernel_banner();

    /* E820 map from bootloader + bitmap PMM for page allocation. */
    memmap_print();
    pmm_init();
    pmm_print_stats();

    /* Demo: allocate one page from the free bitmap. */
    {
        uint32_t page = pmm_alloc_page();
        if (page != 0) {
            serial_puts("[pmm] test alloc page @ ");
            serial_print_hex32(page);
            serial_putln("");
            pmm_free_page(page);
            serial_putln("[pmm] test page freed");
        }
    }

    /* CPU exceptions — must be installed before hardware IRQs can fire safely. */
    idt_init();
    serial_putln("[kmain] IDT loaded");
    vga_write_at(11, 0, "IDT: CPU exceptions 0-31 installed           ");

    /* PIC remap, IRQ gates, keyboard, STI — typing now generates IRQ1. */
    irq_init();
    vga_write_at(17, 0, "Interrupts ON — type on bottom line (row 24)   ");

    serial_putln("[kmain] idle — press keys in QEMU window (focus required)");

    /*
     * HLT until the next interrupt (keyboard IRQ wakes us).
     * Without STI + unmasked IRQ1, the CPU would sleep forever.
     */
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}
