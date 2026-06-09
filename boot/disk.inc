; INT 13h EDD (AH=41h / AH=42h) disk I/O.

check_edd:
    mov dl, [boot_drive]
    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13
    jc .fail

    cmp bx, 0xAA55
    jne .fail

    test cx, 0x0004
    jz .fail

    clc
    ret

.fail:
    stc
    ret

load_kernel_edd:
    mov dl, [boot_drive]
    mov word [disk_packet + 2], KERNEL_SECTORS
    mov word [disk_packet + 4], KERNEL_LOAD_OFF
    mov word [disk_packet + 6], KERNEL_LOAD_SEG
    mov dword [disk_packet + 8], KERNEL_LBA_START
    mov dword [disk_packet + 12], 0

    mov ah, 0x42
    mov si, disk_packet
    int 0x13
    ret
