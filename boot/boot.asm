; Stage-1 bootloader: E820 map, load kernel, A20, protected mode.

%include "constants.inc"

org BOOTLOADER_ORG
bits 16

jmp start

%include "print16.inc"
%include "a20.inc"
%include "disk.inc"
%include "gdt.inc"
%include "e820.inc"

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, BOOTLOADER_ORG

    mov [boot_drive], dl

%ifdef DEBUG_SERIAL
    mov si, msg_start
    call print_string
%endif

    call check_edd
    jc edd_unavailable

%ifdef DEBUG_SERIAL
    mov si, msg_edd_ok
    call print_string
%endif

    call load_kernel_edd
    jc load_failed

    call detect_memory_e820

%ifdef DEBUG_SERIAL
    mov si, msg_loaded
    call print_string
    call enable_a20
    jc a20_failed
    mov si, msg_a20_ok
    call print_string
    mov si, msg_pm
    call print_string
    jmp do_pm
%else
    call enable_a20
    jc a20_failed
do_pm:
%endif

    call enter_protected_mode

edd_unavailable:
    mov si, msg_no_edd
    call print_string
    jmp $

load_failed:
    mov si, msg_load_err
    call print_string
    jmp $

a20_failed:
    mov si, msg_a20_err
    call print_string
    jmp $

boot_drive: db 0

disk_packet:
    db 0x10
    db 0
    dw KERNEL_SECTORS
    dw KERNEL_LOAD_OFF
    dw KERNEL_LOAD_SEG
    dd KERNEL_LBA_START
    dd 0

%ifdef DEBUG_SERIAL
msg_start    db 'EDD...', 13, 10, 0
msg_edd_ok   db 'EDD OK', 13, 10, 0
msg_loaded   db 'Loaded 10000h', 13, 10, 0
msg_a20_ok   db 'A20 OK', 13, 10, 0
msg_pm       db 'PM', 13, 10, 0
%endif
msg_no_edd   db 'No EDD', 13, 10, 0
msg_load_err db 'Read err', 13, 10, 0
msg_a20_err  db 'A20 err', 13, 10, 0

%include "pm.inc"

times 510-($-$$) db 0
dw 0xAA55
