; Flat GDT: null, 32-bit code/data (protected mode), 64-bit code (long mode).

gdt_start:
    dq 0

gdt_code:
    dw 0xFFFF
    dw 0
    db 0
    db 10011010b
    db 11001111b
    db 0

gdt_data:
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0

gdt_code64:
    dw 0xFFFF
    dw 0
    db 0
    db 10011010b
    db 10101111b
    db 0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
