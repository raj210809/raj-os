; Real-mode output: INT 10h in normal builds, serial-only when DEBUG_SERIAL is set.

%ifdef DEBUG_SERIAL
%include "serial16.inc"

print_string:
    jmp serial_string

print_newline:
    jmp serial_newline

%else

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

%endif
