; Minimal flat GDT for 32-bit protected mode.
; Code and data segments both cover the full 4 GiB address space (base 0).

gdt_start:
    dq 0                         ; [0] null descriptor (required)

gdt_code:
    dw 0xFFFF                    ; limit 15:0
    dw 0                         ; base 15:0
    db 0                         ; base 23:16
    db 10011010b                 ; present, ring 0, code, executable, readable
    db 11001111b                 ; 4 KiB granularity, 32-bit, limit 19:16
    db 0                         ; base 31:24

gdt_data:
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b                 ; present, ring 0, data, writable
    db 11001111b
    db 0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
