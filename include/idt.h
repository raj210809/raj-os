/*
 * idt.h — Interrupt Descriptor Table (IDT) for 64-bit long mode.
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

/* 64-bit code segment selector (GDT entry 3, see boot/gdt.inc). */
#define IDT_CODE64_SEL          0x18

/*
 * Stack frame built by kernel/arch/idt_stubs.asm before calling C.
 * Layout matches push order in isr_common_stub (RAX first, R15 last).
 */
typedef struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no;
    uint64_t err_code;
    /* CPU always pushes these five in long mode (even from ring 0). */
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) registers_t;

/* Install 256 IDT gates; vectors 0–31 point at our ISR stubs. */
void idt_init(void);

/* Register one interrupt gate (hardware IRQ stubs use this after PIC remap). */
void idt_register_handler(uint8_t vector, void (*handler)(void));

/* C handler invoked from assembly for every exception / IRQ stub we wire up. */
void exception_handler(registers_t *regs);

/* Demo helpers — trigger faults on purpose. */
void idt_test_ud2(void);
void idt_test_divide_by_zero(void);

#endif /* IDT_H */
