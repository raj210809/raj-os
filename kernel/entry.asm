; =============================================================================
; entry.asm — 32-bit kernel bootstrap (first code at physical 0x10000).
; =============================================================================
;
; Responsibilities:
;   1. Stack setup   — SS:ESP at PM_STACK_TOP (16-byte aligned for C ABI).
;   2. BSS zeroing   — .bss is not in the disk image; clear it before C runs.
;   3. Segment setup — reload flat data segments.
;   4. Early serial  — optional debug trace before calling C.
;   5. Call kmain    — transfer control to kernel/main.c.
; =============================================================================

%include "constants.inc"

bits 32

section .text
global kernel_main

extern kmain
extern __bss_start
extern __bss_end

; First instruction at 0x10000 — jump over included helpers in .text.
jmp kernel_main

kernel_main:
    ; -------------------------------------------------------------------------
    ; Stack setup
    ; -------------------------------------------------------------------------
    ; The x86 stack grows *downward*: pushes decrement ESP.
    ; SS:ESP must point to the top (highest address) of the stack region.
    ;
    ; We use PM_STACK_TOP (0x90000), below 1 MiB but away from the kernel
    ; image at 0x10000.  The bootloader used this address briefly in pm.inc;
    ; we reload it here so C code has a known, spacious stack.
    ; -------------------------------------------------------------------------

    mov ax, GDT_DATA_SEL           ; flat data segment selector from GDT
    mov ds, ax                     ; all data segments must match for C pointers
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax                     ; stack segment = same flat data segment

    mov esp, PM_STACK_TOP          ; dedicated stack in low memory (not on disk)
    and esp, 0xFFFFFFF0            ; 16-byte alignment required by cdecl ABI

    ; -------------------------------------------------------------------------
    ; Zero .bss (uninitialized globals — not present in kernel.bin on disk)
    ; -------------------------------------------------------------------------
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi                   ; ECX = byte count (may be 0)
    jz .bss_done
    xor eax, eax                   ; fill with 0
    rep stosb
.bss_done:

    ; -------------------------------------------------------------------------
    ; Debug: announce asm entry on COM1 (useful when VGA is not ready yet).
    ; -------------------------------------------------------------------------
    mov esi, msg_asm_entry
    call serial_print32
    call serial_newline32

    mov esi, msg_stack
    call serial_print32
    mov eax, esp
    call serial_print_hex32_eax
    call serial_newline32

    ; -------------------------------------------------------------------------
    ; VGA sanity check (before C): write '!' at top-left of 0xB8000.
    ; If the QEMU window stays black but serial works, fix -machine/-vga in
    ; the Makefile (see QEMU_MACHINE / QEMU_VGA).
    ; -------------------------------------------------------------------------
    mov edi, VGA_PHYS
    mov word [edi], 0x0F21          ; attribute 0x0F, character '!'

    ; -------------------------------------------------------------------------
    ; Enter the C kernel — kmain() in kernel/main.c
    ; -------------------------------------------------------------------------
    mov esi, msg_call_kmain
    call serial_print32
    call serial_newline32

    call kmain

    ; kmain should not return; if it does, report and halt.
    mov esi, msg_kmain_return
    call serial_print32
    call serial_newline32

.halt:
    hlt
    jmp .halt

; --- Read-only strings -------------------------------------------------------

msg_asm_entry:    db '[entry] 32-bit kernel bootstrap', 0
msg_stack:        db '[entry] ESP (stack top) = ', 0
msg_call_kmain:   db '[entry] calling kmain()', 0
msg_kmain_return: db '[entry] kmain returned — halting', 0

; Serial helpers live after the entry path so they are not placed at 0x10000.
%include "serial32.inc"
