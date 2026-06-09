; Shared constants for bootloader and kernel.
%ifndef CONSTANTS_INC
%define CONSTANTS_INC

; --- Memory map --------------------------------------------------------------
%define BOOTLOADER_ORG   0x7C00
%define KERNEL_PHYS      0x10000
%define KERNEL_LOAD_SEG  0x1000
%define KERNEL_LOAD_OFF  0x0000
; Temporary stack used only inside boot/pm.inc before the kernel runs.
%define PM_STACK_TOP     0x90000

; C kernel reuses this address in kernel/entry.asm (see boot/pm.inc).
%define VGA_PHYS         0xB8000

; Bootloader E820 map (see include/memmap.h). PMM bitmap lives in kernel BSS.
%define MEMMAP_PHYS      0x8000

; --- Disk layout -------------------------------------------------------------
%define KERNEL_LBA_START 1
%define KERNEL_SECTORS   24

; --- GDT segment selectors (index << 3) --------------------------------------
%define GDT_NULL_SEL     0x00
%define GDT_CODE_SEL     0x08
%define GDT_DATA_SEL     0x10

%endif
