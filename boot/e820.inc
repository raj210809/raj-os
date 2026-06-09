; =============================================================================
; e820.inc — BIOS INT 15h AX=E820h memory map (real mode only).
; =============================================================================

%ifndef E820_INC
%define E820_INC

%define MEMMAP_PHYS      0x8000
%define MEMMAP_MAX       20
%define E820_ENTRY_SIZE  24

detect_memory_e820:
    push eax
    push ebx
    push ecx
    push edx
    push di

    xor ax, ax
    mov es, ax

    ; Clear the whole map buffer (count + entries).
    mov di, MEMMAP_PHYS
    mov cx, 256
    xor ax, ax
    rep stosw

    mov di, MEMMAP_PHYS + 4
    xor ebx, ebx
    mov edx, 0x0534D4150
    mov ecx, E820_ENTRY_SIZE

.loop:
    xor ax, ax
    mov es, ax

    mov eax, 0xE820
    int 0x15
    jc .done
    cmp eax, 0x534D4150
    jne .done

    inc dword [MEMMAP_PHYS]
    cmp dword [MEMMAP_PHYS], MEMMAP_MAX
    ja .done

    add di, E820_ENTRY_SIZE
    test ebx, ebx
    jnz .loop

.done:
    pop di
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret

%endif
