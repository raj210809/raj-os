; =============================================================================
; idt_stubs.asm — ISR trampolines for CPU exceptions (vectors 0–31).
; =============================================================================
; The CPU jumps here with CS:EIP/EFLAGS [+ error code] on the stack.
; We save registers, call exception_handler() in C, restore, and IRET.
;
; Two macro kinds:
;   ISR_NOERRCODE — CPU did not push an error code; we push a dummy 0.
;   ISR_ERRCODE   — CPU already pushed error code; we only push vector #.
; =============================================================================

%include "constants.inc"

bits 32

; --- Stub per vector ---------------------------------------------------------

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0                 ; dummy error code (uniform stack frame)
    push dword %1                ; interrupt / exception number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push dword %1                ; vector (real error code is already on stack)
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; --- Common path to C ----------------------------------------------------------

global isr_common_stub
isr_common_stub:
    ; Save DS and reload flat data segments for C (matches registers_t layout).
    push ds
    mov ax, GDT_DATA_SEL
    mov ds, ax
    mov es, ax

    pusha                        ; eax..edi at bottom of frame (ESP points at EDI)

    push esp                     ; &registers_t for exception_handler()
    extern exception_handler
    call exception_handler
    add esp, 4

    popa
    pop ds

    add esp, 8                   ; remove dummy/real err_code + vector number
    iret

; =============================================================================
; Hardware IRQ stubs (IDT vectors 32–47 after PIC remap).
; =============================================================================

%macro IRQ 1
global irq%1
irq%1:
    push dword 0                 ; IRQs have no CPU error code
    push dword %1                ; IDT vector number
    jmp irq_common_stub
%endmacro

IRQ 32
IRQ 33
IRQ 34
IRQ 35
IRQ 36
IRQ 37
IRQ 38
IRQ 39
IRQ 40
IRQ 41
IRQ 42
IRQ 43
IRQ 44
IRQ 45
IRQ 46
IRQ 47

global irq_common_stub
irq_common_stub:
    push ds
    mov ax, GDT_DATA_SEL
    mov ds, ax
    mov es, ax

    pusha

    push esp
    extern irq_handler
    call irq_handler
    add esp, 4

    popa
    pop ds

    add esp, 8
    iret
