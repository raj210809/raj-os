/*
 * main.c — 64-bit kernel with PMM, VMM, kmalloc, PIT timer, and keyboard IRQs.
 */

#include "idt.h"
#include "irq.h"
#include "kmalloc.h"
#include "kmalloc_test.h"
#include "memmap.h"
#include "pit.h"
#include "pmm.h"
#include "serial.h"
#include "vga.h"
#include "vmm.h"
#include "vmm_test.h"

static void kernel_banner(void)
{
    vga_puts("C Kernel 64 bit ka yaha se suru hota hai\n\n");
    vga_puts("Boot:     real mode se protected se PAE + PML4\n");
    vga_puts("Paging:   VMM 4 KiB pages, 128 MiB identity\n");
    vga_puts("PMM:      bitmap, 4 KiB frames from E820\n");
    vga_puts("Heap:     kmalloc/kfree (linked-list free blocks)\n");
    vga_puts("IRQ:      timer IRQ0 (~100 Hz) + keyboard IRQ1\n");
    vga_puts("VGA:      text driver @ 0xB8000\n");
    vga_puts("Kernel:   linked at 0x10000\n");
    vga_puts("\n");
    vga_write_at(8, 0, "Mode: long mode (CR0.PG=1, EFER.LME=1)");
}

void kmain(void)
{
    serial_putln("[boot] long mode OK");
    serial_putln("[entry] 64-bit kernel bootstrap");
    serial_putln("[kmain] entered 64-bit C kernel");

    vga_init();
    serial_putln("[kmain] VGA ready");

    kernel_banner();

    memmap_print();
    pmm_init();
    pmm_print_stats();

    vmm_init();
    vmm_run_tests();

    kmalloc_init();
    kmalloc_run_tests();

    idt_init();
    idt_test_ud2();

    irq_init();

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_write_at(16, 0, "Live: timer row 11 | heap row 13 | VMM row 14       ");
    vga_write_at(18, 0, "Serial: [timer] every 100 ticks | test logs        ");

    serial_putln("[kmain] idle — timer on VGA row 11, type on row 24");

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}
