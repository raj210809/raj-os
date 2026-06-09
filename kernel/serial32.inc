; =============================================================================
; serial32.inc — 32-bit COM1 serial output (protected mode, flat segments).
; =============================================================================
; UART base 0x3F8 (COM1).  Used for early debug before VGA is initialized.
; Each character waits for the transmit-hold-empty bit in the line status reg.
; =============================================================================

; void serial_puts(const char *s) — cdecl wrapper for C (argument on stack).
global serial_puts
global serial_putln
global serial_putchar

serial_putchar:
    push ebp
    mov ebp, esp
    movzx eax, byte [ebp + 8]
    call serial_putchar32
    pop ebp
    ret

serial_puts:
    push ebp
    mov ebp, esp
    push esi                       ; cdecl: preserve caller's ESI
    mov esi, [ebp + 8]             ; const char *s
    call serial_print32
    pop esi
    pop ebp
    ret

serial_putln:
    push ebp
    mov ebp, esp
    push esi
    mov esi, [ebp + 8]
    call serial_print32
    call serial_newline32
    pop esi
    pop ebp
    ret

serial_putchar32:
    push ebx
    mov bl, al
    mov dx, 0x3FD
.wait:
    in al, dx
    test al, 0x20
    jz .wait
    mov al, bl
    mov dx, 0x3F8
    out dx, al
    pop ebx
    ret

serial_print32:
    push eax
    push esi
.loop:
    lodsb
    test al, al
    jz .done
    call serial_putchar32
    jmp .loop
.done:
    pop esi
    pop eax
    ret

serial_newline32:
    mov al, 13
    call serial_putchar32
    mov al, 10
    jmp serial_putchar32

; void serial_print_hex32(uint32_t value) — cdecl, for C.
global serial_print_hex32
serial_print_hex32:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    call serial_print_hex32_eax
    pop ebp
    ret

; Print the 32-bit value already in EAX (for asm callers like entry.asm).
global serial_print_hex32_eax
serial_print_hex32_eax:
    push eax
    push ebx
    push ecx
    mov ebx, eax
    mov ecx, 8
.loop:
    mov eax, ebx
    shr eax, 28                  ; top nibble of the remaining bits
    call .emit_hex_digit
    shl ebx, 4
    loop .loop
    pop ecx
    pop ebx
    pop eax
    ret

.emit_hex_digit:
    and eax, 0x0f
    cmp al, 9
    jbe .digit
    add al, 'A' - 10
    jmp .out
.digit:
    add al, '0'
.out:
    jmp serial_putchar32
