; 64-bit COM1 serial (System V AMD64 ABI for C).

global serial_puts
global serial_putln
global serial_print_hex32

serial_puts:
    push rbp
    mov rbp, rsp
    push rsi
    mov rsi, rdi
    call serial_print64
    pop rsi
    pop rbp
    ret

serial_putln:
    push rbp
    mov rbp, rsp
    push rsi
    mov rsi, rdi
    call serial_print64
    call serial_newline64
    pop rsi
    pop rbp
    ret

serial_print_hex32:
    push rbp
    mov rbp, rsp
    mov eax, edi
    call serial_print_hex32_eax
    pop rbp
    ret

serial_putchar64:
    mov rdx, 0x3FD
.wait:
    in al, dx
    test al, 0x20
    jz .wait
    mov rdx, 0x3F8
    mov al, sil
    out dx, al
    ret

serial_print64:
    push rbx
    push rsi
.loop:
    movzx ebx, byte [rsi]
    test bl, bl
    jz .done
    mov sil, bl
    call serial_putchar64
    inc rsi
    jmp .loop
.done:
    pop rsi
    pop rbx
    ret

serial_newline64:
    mov sil, 13
    call serial_putchar64
    mov sil, 10
    jmp serial_putchar64

global serial_print_hex32_eax
serial_print_hex32_eax:
    push rbx
    push rcx
    mov ebx, eax
    mov ecx, 8
.loop:
    mov eax, ebx
    shr eax, 28
    call .emit_hex_digit
    shl ebx, 4
    loop .loop
    pop rcx
    pop rbx
    ret

.emit_hex_digit:
    and eax, 0x0F
    cmp al, 9
    jbe .digit
    add al, 'A' - 10
    jmp .out
.digit:
    add al, '0'
.out:
    mov sil, al
    jmp serial_putchar64
