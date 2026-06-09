; =============================================================================
; Test sector for Exercise 3 — becomes LBA 1 on the disk image.
; After the bootloader loads it, inspect physical 0x10000 in GDB.
; =============================================================================

org 0x0000
bits 16

    ; Magic 32-bit marker at the very start — easy to spot with x/8x 0x10000
    dd 0xDEADBEEF
    dd 0xCAFEBABE

    db 'Hello from LBA 1! These bytes live at physical 0x10000.', 0

    ; Fill rest of sector so `x/512xb 0x10000` shows a full 512-byte block.
    times 512-($-$$) db 0xAA
