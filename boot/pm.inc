; Switch from real mode to 32-bit protected mode, then jump to the kernel.
; Does not return.
;
; Stack note: PM_STACK_TOP is a *temporary* stack for the first instructions in
; protected mode.  kernel/entry.asm replaces it with the linker-reserved C stack
; before calling kmain().

enter_protected_mode:
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1                    ; set PE (Protection Enable)
    mov cr0, eax

    ; Far jump flushes the prefetch queue and loads CS with the code selector.
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

    ; Absolute far jump — plain "jmp 0x10000" assembles as a relative near jmp!
    jmp GDT_CODE_SEL:KERNEL_PHYS
