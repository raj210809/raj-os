; 64-bit kernel bootstrap at physical 0x10000.

%include "constants.inc"

bits 64

section .text
global kernel_main64

extern kmain
extern __bss_start
extern __bss_end

kernel_main64:
    mov rax, PM_STACK_TOP
    mov rsp, rax
    and rsp, ~0xF

    mov rdi, __bss_start
    mov rcx, __bss_end
    sub rcx, rdi
    jz .bss_done
    xor rax, rax
    rep stosb
.bss_done:
    call kmain

.halt:
    hlt
    jmp .halt
