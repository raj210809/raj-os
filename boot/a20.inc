; Enable A20: fast port 0x92, then BIOS INT 15h AX=2401 as fallback.

enable_a20:
    call a20_try_fast
    call a20_test
    jnc .ok
    call a20_try_bios
    call a20_test
    jnc .ok
    stc
    ret
.ok:
    clc
    ret

a20_try_fast:
    in al, 0x92
    test al, 2
    jnz .done
    or al, 2
    and al, 0xFE
    out 0x92, al
.done:
    ret

a20_try_bios:
    mov ax, 0x2401
    int 0x15
    ret

; Compare bytes at 0000:0500 vs FFFF:0010 — same physical addr if A20 is off.
a20_test:
    pushf
    push ds
    push es
    push di
    cli
    xor ax, ax
    mov es, ax
    mov ax, 0xFFFF
    mov ds, ax
    mov di, 0x0500
    mov word [es:di], 0
    mov word [ds:0x0010], 0xFFFF
    cmp word [es:di], 0
    jne .disabled
    clc
    jmp .out
.disabled:
    stc
.out:
    pop di
    pop es
    pop ds
    popf
    ret
