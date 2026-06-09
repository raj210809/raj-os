; COM1 serial — compact 16-bit (QEMU -serial stdio).

serial_putchar:
    push bx
    mov bl, al
    mov dx, 0x3FD
.wait:
    in al, dx
    test al, 0x20
    jz .wait
    mov al, bl
    mov dx, 0x3F8
    out dx, al
    pop bx
    ret

serial_string:
    push ax
    push si
.loop:
    lodsb
    test al, al
    jz .done
    call serial_putchar
    jmp .loop
.done:
    pop si
    pop ax
    ret

serial_newline:
    mov al, 13
    call serial_putchar
    mov al, 10
    jmp serial_putchar
