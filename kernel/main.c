/*
 * main.c — 64-bit kernel (phases A–D: long mode, VGA, IDT, IRQ/keyboard).
 */

#include "idt.h"
#include "irq.h"
#include "serial.h"
#include "vga.h"

static void kernel_banner(void)
{
    vga_puts("=== C Kernel (64-bit Long Mode) ===\n\n");
    vga_puts("Boot:     real mode -> protected -> PAE + PML4\n");
    vga_puts("Paging:   identity map, 128 MiB (2 MiB pages)\n");
    vga_puts("IDT:      vectors 0-31 (exceptions), 32-47 (IRQ)\n");
    vga_puts("VGA:      text driver @ 0xB8000\n");
    vga_puts("Kernel:   linked at 0x10000\n");
    vga_puts("\n");
    vga_write_at(8, 0, "Mode: long mode (CR0.PG=1, EFER.LME=1)");
    vga_write_at(10, 0, "Next:   PMM / E820 (phase E)");
}

void kmain(void)
{
    serial_putln("[boot] long mode OK");
    serial_putln("[entry] 64-bit kernel bootstrap");
    serial_putln("[kmain] entered 64-bit C kernel");

    vga_init();
    serial_putln("[kmain] VGA ready");

    kernel_banner();

    idt_init();
    idt_test_ud2();

    irq_init();

    serial_putln("[kmain] idle — type in QEMU window (IRQ1 -> VGA row 24)");

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}
