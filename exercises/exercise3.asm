; =============================================================================
; Exercise 3 — Load one sector to physical 0x10000, then halt for GDB
; =============================================================================
; Physical address 0x10000 in real mode is accessed as segment:offset
;   1000:0000   (because 0x1000 * 16 + 0 = 0x10000)
;
; We use the classic INT 13h AH=02 (BIOS read sectors) — the old CHS style.
; That is simpler than EDD (AH=42) and fine for one sector near the start
; of the disk.
;
; Disk layout (see Makefile):
;   LBA 0  = this boot sector (at 0x7C00 when running)
;   LBA 1  = test_data.bin  (loaded to 0x10000)
;
; Run under GDB:  make exercise3-gdb
; Then in another terminal:
;   gdb -ex 'target remote :1234' \
;       -ex 'x/8x 0x10000' \
;       -ex 'x/32cb 0x10000'
; =============================================================================

org 0x7c00
bits 16

%define LOAD_SEG  0x1000
%define LOAD_OFF  0x0000

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [boot_drive], dl

    mov si, msg_start
    call print_string

    ; ES:BX = destination. INT 13h AH=02 uses ES:BX, not DS:BX.
    mov ax, LOAD_SEG
    mov es, ax
    xor bx, bx                     ; BX = 0 -> ES:BX = 1000:0000 = 0x10000

    call load_one_sector
    jc load_failed

    mov si, msg_ok
    call print_string

    mov si, msg_gdb
    call print_string

    ; Halt forever — QEMU stays paused if started with -S, giving you time
    ; to inspect 0x10000 in GDB.
    jmp $

load_failed:
    mov si, msg_err
    call print_string
    jmp $

; INT 13h AH=02 — Read disk sectors (CHS addressing).
; Loads 1 sector: cylinder 0, head 0, sector 2 (1-based sector numbering).
; Sector 1 on disk = our boot sector; sector 2 = the data we want at 0x10000.
load_one_sector:
    mov ah, 0x02                   ; BIOS: read sectors
    mov al, 1                      ; count: 1 sector
    mov ch, 0                      ; cylinder 0 (low 8 bits)
    mov dh, 0                      ; head 0
    mov cl, 2                      ; sector 2 (sectors are 1-based on disk)
    mov dl, [boot_drive]
    int 0x13
    ret                            ; carry set on error

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

boot_drive: db 0

msg_start db 'Exercise 3: loading LBA 1 -> 1000:0 (phys 0x10000)...', 13, 10, 0
msg_ok    db 'Load OK. Inspect memory at 0x10000 in GDB.', 13, 10, 0
msg_gdb   db 'Try: x/8x 0x10000   and   x/s 0x10000', 13, 10, 0
msg_err   db 'INT 13h read failed.', 13, 10, 0

times 510-($-$$) db 0
dw 0xaa55
