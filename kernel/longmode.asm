; PAE + PML4 identity map (128 MiB) + enter 64-bit long mode.
; 32-bit code lives in .text32; 64-bit entry in .text.

%include "constants.inc"

section .text32
bits 32

global enter_long_mode

%define PTE_HUGE_2MB  0x83

enter_long_mode:
    mov edi, PT_PD_PHYS
    xor eax, eax

.fill_pd:
    mov ecx, eax
    shl ecx, 21
    or ecx, PTE_HUGE_2MB
    mov [edi + eax * 8], ecx
    mov dword [edi + eax * 8 + 4], 0
    inc eax
    cmp eax, 64
    jb .fill_pd

    mov eax, PT_PD_PHYS | 3
    mov dword [PT_PDPT_PHYS], eax
    mov dword [PT_PDPT_PHYS + 4], 0

    mov eax, PT_PDPT_PHYS | 3
    mov dword [PT_PML4_PHYS], eax
    mov dword [PT_PML4_PHYS + 4], 0

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov eax, PT_PML4_PHYS
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    jmp GDT_CODE64_SEL:long_mode_entry

section .text
bits 64

extern kernel_main64

long_mode_entry:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov rsp, PM_STACK_TOP
    and rsp, ~0xF

    jmp kernel_main64
