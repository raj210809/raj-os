; Real mode -> 32-bit protected mode -> kernel at KERNEL_PHYS (32-bit entry).
; The kernel entry stub enables PAE/PML4 and switches to long mode.

enter_protected_mode:
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp GDT_CODE_SEL:pm_entry

bits 32
pm_entry:
    mov ax, GDT_DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, PM_STACK_TOP

    jmp GDT_CODE_SEL:KERNEL_PHYS
