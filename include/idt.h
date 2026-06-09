/*
 * idt.h — Interrupt Descriptor Table (IDT) for 32-bit protected mode.
 *
 * In real mode the BIOS handled CPU exceptions via INT 0-31.  In our kernel
 * we install our own handlers so faults (divide by zero, invalid opcode, etc.)
 * jump to kernel code instead of triple-faulting / resetting QEMU.
 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* CPU exception vectors (architecturally defined 0–31). */
#define EXC_DIVIDE_BY_ZERO      0
#define EXC_DEBUG               1
#define EXC_NMI                 2
#define EXC_BREAKPOINT          3
#define EXC_OVERFLOW            4
#define EXC_BOUND_RANGE         5
#define EXC_INVALID_OPCODE      6   /* UD2 instruction fires this */
#define EXC_DEVICE_NOT_AVAIL    7
#define EXC_DOUBLE_FAULT        8
#define EXC_INVALID_TSS         10
#define EXC_SEGMENT_NOT_PRESENT 11
#define EXC_STACK_FAULT         12
#define EXC_GENERAL_PROTECTION  13
#define EXC_PAGE_FAULT          14

/*
 * Stack frame built by kernel/arch/idt_stubs.asm before calling C.
 * Layout matches the order of pushes in isr_common_stub.
 */
/*
 * Stack frame at ESP when exception_handler() runs (see idt_stubs.asm):
 *   [edi..eax] pusha, [ds], [int_no], [err_code], [eip, cs, eflags] from CPU.
 */
typedef struct registers {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t ds;
    uint32_t int_no;
    uint32_t err_code;
    uint32_t eip, cs, eflags;
} __attribute__((packed)) registers_t;

/* Install 256 IDT gates; vectors 0–31 point at our ISR stubs. */
void idt_init(void);

/* Register one interrupt gate (hardware IRQ stubs use this after PIC remap). */
void idt_register_handler(uint8_t vector, void (*handler)(void));

/* C handler invoked from assembly for every exception / IRQ stub we wire up. */
void exception_handler(registers_t *regs);

/* Demo helpers (defined in main.c) — trigger faults on purpose. */
void idt_test_ud2(void);
void idt_test_divide_by_zero(void);

#endif /* IDT_H */
