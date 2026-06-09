/*
 * linker.ld — 64-bit kernel at physical 0x10000 (identity-mapped by bootloader).
 */

OUTPUT_FORMAT(elf64-x86-64)
ENTRY(enter_long_mode)

SECTIONS
{
    . = 0x10000;

    .text32 : ALIGN(4) {
        *(.text32)
    }

    .text : ALIGN(4) {
        *(.text .text.*)
    }

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(4) {
        *(.data .data.*)
    }

    .bss : ALIGN(16) {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
}
