; =============================================================================
; Exercise 1 — What does BIOS give you?
; =============================================================================
; When the BIOS finishes loading the boot sector, it jumps to 0x7C00 and
; leaves DL = boot drive number. Everything else is *not* guaranteed.
;
; This exercise prints CS, DS, ES, and SS *immediately*, before we change
; anything, so you can see the real values your firmware picked.
;
; Run:  make exercise1-run
;
; Typical QEMU/BIOS results (one of these CS/IP pairs lands at physical 0x7C00):
;   CS=0000  IP=7C00   (linear address = CS*16 + IP = 0x7C00)
;   CS=07C0  IP=0000   (same physical address, different segment:offset view)
; DS, ES, SS are often garbage until you set them — that is the lesson.
; =============================================================================

org 0x7c00
bits 16

start:
    ; --- Do NOT touch DS/ES/SS yet. Read them first. -----------------------

    mov si, hdr
    call print_string

    mov si, lbl_cs
    call print_string
    mov ax, cs
    call print_hex_word
    call print_newline

    mov si, lbl_ds
    call print_string
    mov ax, ds
    call print_hex_word
    call print_newline

    mov si, lbl_es
    call print_string
    mov ax, es
    call print_hex_word
    call print_newline

    mov si, lbl_ss
    call print_string
    mov ax, ss
    call print_hex_word
    call print_newline

    ; --- Bonus: IP and SP (IP has no mov instruction — use call/pop trick). -
    mov si, lbl_ip
    call print_string
    call .ip_here                  ; pushes address of next instruction
.ip_here:
    pop ax                         ; AX = offset where BIOS left us (~0x7Cxx)
    call print_hex_word
    call print_newline

    mov si, lbl_sp
    call print_string
    mov ax, sp
    call print_hex_word
    call print_newline

    mov si, lbl_boot
    call print_string
    mov al, dl
    call print_hex_byte
    call print_newline

    mov si, footer
    call print_string

    jmp $

; ---------------------------------------------------------------------------
; print_string — INT 10h teletype (AH=0Eh). SI -> null-terminated string.
; ---------------------------------------------------------------------------
print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0e
    mov bh, 0
    int 0x10
    jmp print_string
.done:
    ret

print_newline:
    mov ah, 0x0e
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    ret

; Print AL as two hex digits.
print_hex_byte:
    mov ah, al
    shr al, 4
    call print_hex_nibble
    mov al, ah
    call print_hex_nibble
    ret

; Print AX as four hex digits (big-endian style: high nibble first).
print_hex_word:
    push ax
    mov al, ah
    call print_hex_byte
    pop ax
    call print_hex_byte
    ret

print_hex_nibble:
    and al, 0x0f
    cmp al, 9
    jbe .digit
    add al, 'A' - 10
    jmp .emit
.digit:
    add al, '0'
.emit:
    mov ah, 0x0e
    mov bh, 0
    int 0x10
    ret

hdr:     db '=== Exercise 1: segment registers at BIOS handoff ===', 13, 10, 0
lbl_cs:  db 'CS=', 0
lbl_ds:  db 'DS=', 0
lbl_es:  db 'ES=', 0
lbl_ss:  db 'SS=', 0
lbl_ip:  db 'IP=', 0
lbl_sp:  db 'SP=', 0
lbl_boot: db 'DL (boot drive)=', 0
footer:  db 13, 10, 'Compare with GDB: info registers', 13, 10, 0

times 510-($-$$) db 0
dw 0xaa55
