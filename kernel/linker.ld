/*
 * linker.ld — Places the kernel at physical 0x10000 (must match KERNEL_PHYS).
 *
 * The bootloader jumps to kernel_main with CS=0x08 and a flat data segment.
 * This script lays out: .text → .rodata → .data → .bss, then reserves an
 * aligned stack region at the end of BSS for the C calling convention.
 */

OUTPUT_FORMAT(elf32-i386)
ENTRY(kernel_main)

SECTIONS
{
    /* Load address agreed with boot/pm.inc and include/constants.inc */
    . = 0x10000;

    .text : ALIGN(4) {
        *(.text .text.*)
    }

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(4) {
        *(.data .data.*)
    }

    /*
     * Uninitialized globals (.bss).  Not stored in kernel.bin — entry.asm zeroes
     * this range before calling kmain().  The C stack lives at PM_STACK_TOP
     * (see entry.asm), separate from the loaded image.
     */
    .bss : ALIGN(16) {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
}
