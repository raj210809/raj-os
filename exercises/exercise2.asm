; =============================================================================
; Exercise 2 — Write directly to video memory (0xB8000)
; =============================================================================
; In text mode, the screen is not magic — it is just RAM at physical 0xB8000.
; Each character cell is TWO bytes:
;   [0] = ASCII character
;   [1] = attribute byte (foreground/background color)
;
; Segment trick: 0xB8000 = segment 0xB800, offset 0
;   (because 0xB800 * 16 = 0xB8000)
;
; We skip INT 10h entirely. No BIOS calls to put characters on screen.
;
; Run:  make exercise2-run
; =============================================================================

org 0x7c00
bits 16

start:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Point ES at the color text buffer.
    mov ax, 0xb800
    mov es, ax
    xor di, di                     ; start at top-left (row 0, col 0)

    mov si, row0_msg
    mov bl, 0x0f                   ; attribute: white on black
    call write_row

    mov di, 160                    ; row 1 = 80 columns * 2 bytes/cell
    mov si, row1_msg
    mov bl, 0x1f                   ; attribute: white on blue
    call write_row

    mov di, 320                    ; row 2
    mov si, row2_msg
    mov bl, 0xe0                   ; attribute: black on yellow
    call write_row

    jmp $

; write_row — copy null-terminated string to [ES:DI] with attribute BL.
; Advances DI by 2 per character.
write_row:
    lodsb
    test al, al
    jz .done
    mov ah, bl
    mov [es:di], ax
    add di, 2
    jmp write_row
.done:
    ret

row0_msg db 'Direct write to 0xB8000 — no INT 10h!', 0
row1_msg db 'Each cell = [ASCII][color byte]', 0
row2_msg db 'Change BL in write_row to see colors.', 0

times 510-($-$$) db 0
dw 0xaa55
