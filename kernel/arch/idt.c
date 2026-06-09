/*
 * idt.c — Build the 64-bit IDT and dispatch CPU exceptions to a C handler.
 */

#include "idt.h"
#include "serial.h"
#include "vga.h"

#include <stddef.h>
#include <stdint.h>

/* One 16-byte IDT gate (Intel SDM Vol.3A §6.14.1, long-mode). */
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

#define ISR_DECL(n) extern void isr##n(void)
ISR_DECL(0);  ISR_DECL(1);  ISR_DECL(2);  ISR_DECL(3);
ISR_DECL(4);  ISR_DECL(5);  ISR_DECL(6);  ISR_DECL(7);
ISR_DECL(8);  ISR_DECL(9);  ISR_DECL(10); ISR_DECL(11);
ISR_DECL(12); ISR_DECL(13); ISR_DECL(14); ISR_DECL(15);
ISR_DECL(16); ISR_DECL(17); ISR_DECL(18); ISR_DECL(19);
ISR_DECL(20); ISR_DECL(21); ISR_DECL(22); ISR_DECL(23);
ISR_DECL(24); ISR_DECL(25); ISR_DECL(26); ISR_DECL(27);
ISR_DECL(28); ISR_DECL(29); ISR_DECL(30); ISR_DECL(31);

static void (*const isr_table[32])(void) = {
    isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
    isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
    isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
    isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
};

static const char *const exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Checked",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};

static void __attribute__((noinline)) idt_set_gate(uint8_t vector, uint64_t handler,
                                                 uint16_t selector, uint8_t flags)
{
    idt[vector].offset_low  = (uint16_t)(handler & 0xFFFFU);
    idt[vector].selector    = selector;
    idt[vector].ist         = 0;
    idt[vector].type_attr   = flags;
    idt[vector].offset_mid  = (uint16_t)((handler >> 16) & 0xFFFFU);
    idt[vector].offset_high = (uint32_t)(handler >> 32);
    idt[vector].zero        = 0;
}

static void idt_zero_table(void)
{
    for (size_t i = 0; i < 256; i++) {
        idt[i].offset_low  = 0;
        idt[i].selector    = 0;
        idt[i].ist         = 0;
        idt[i].type_attr   = 0;
        idt[i].offset_mid  = 0;
        idt[i].offset_high = 0;
        idt[i].zero        = 0;
    }
}

void idt_init(void)
{
    serial_putln("[idt] building table...");

    idtp.limit = (uint16_t)(sizeof(idt) - 1);
    idtp.base  = (uint64_t)(uintptr_t)&idt[0];

    idt_zero_table();

    /* 0x8E = present, DPL 0, 64-bit interrupt gate. */
    for (uint8_t i = 0; i < 32; i++) {
        idt_set_gate(i, (uint64_t)(uintptr_t)isr_table[i], IDT_CODE64_SEL, 0x8E);
    }

    serial_putln("[idt] lidt...");
    __asm__ __volatile__("lidt %0" : : "m"(idtp));
    __asm__ __volatile__("cli");

    serial_putln("[idt] loaded vectors 0-31 (exceptions)");
}

void idt_register_handler(uint8_t vector, void (*handler)(void))
{
    idt_set_gate(vector, (uint64_t)(uintptr_t)handler, IDT_CODE64_SEL, 0x8E);
}

static const char *exception_name(uint32_t vector)
{
    if (vector < 32U) {
        return exception_messages[vector];
    }
    return "IRQ / reserved";
}

static void report_exception(registers_t *regs)
{
    const char *name = exception_name((uint32_t)regs->int_no);

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_write_at(12, 0, "!!! CPU EXCEPTION — kernel caught it !!!       ");
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);

    vga_write_at(14, 0, "Vector: ");
    vga_write_at(15, 0, "Name:   ");
    vga_write_at(16, 0, "RIP:    ");
    vga_write_at(17, 0, "ERR:    ");

    {
        char buf[8];
        uint8_t v = (uint8_t)regs->int_no;
        buf[0] = (char)('0' + (v / 10U) % 10U);
        buf[1] = (char)('0' + v % 10U);
        buf[2] = '\0';
        vga_write_at(14, 9, buf);
    }

    vga_write_at(15, 9, name);

    serial_puts("[exception] vector=");
    serial_print_hex32((uint32_t)regs->int_no);
    serial_puts(" err=");
    serial_print_hex32((uint32_t)regs->err_code);
    serial_puts(" rip=");
    serial_print_hex64(regs->rip);
    serial_puts(" — ");
    serial_puts(name);
    serial_putln("");
}

void exception_handler(registers_t *regs)
{
    report_exception(regs);

    switch (regs->int_no) {
    case EXC_INVALID_OPCODE:
        regs->rip += 2;
        serial_putln("[exception] UD2 skipped — execution continues");
        vga_write_at(19, 0, "Recovered: UD2 (invalid opcode) skipped.       ");
        return;

    case EXC_BREAKPOINT:
        regs->rip += 1;
        serial_putln("[exception] breakpoint skipped");
        return;

    case EXC_DIVIDE_BY_ZERO:
        serial_putln("[exception] divide by zero — halted (no reboot)");
        vga_write_at(19, 0, "Halted: divide by zero (reset QEMU to retry).  ");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        for (;;) {
            __asm__ __volatile__("hlt");
        }

    default:
        serial_putln("[exception] unhandled — halted");
        vga_write_at(19, 0, "Halted: unhandled exception.                   ");
        for (;;) {
            __asm__ __volatile__("hlt");
        }
    }
}
