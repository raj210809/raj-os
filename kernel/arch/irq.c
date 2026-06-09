/*
 * irq.c — Hardware IRQ dispatch (IDT vectors 32–47).
 */

#include "idt.h"
#include "io.h"
#include "irq.h"
#include "keyboard.h"
#include "pic.h"
#include "serial.h"

#define IRQ_COUNT 16

/* IRQ stub symbols from idt_stubs.asm (vectors 32–47). */
#define IRQ_DECL(n) extern void irq##n(void)
IRQ_DECL(32); IRQ_DECL(33); IRQ_DECL(34); IRQ_DECL(35);
IRQ_DECL(36); IRQ_DECL(37); IRQ_DECL(38); IRQ_DECL(39);
IRQ_DECL(40); IRQ_DECL(41); IRQ_DECL(42); IRQ_DECL(43);
IRQ_DECL(44); IRQ_DECL(45); IRQ_DECL(46); IRQ_DECL(47);

static void (*const irq_stub_table[IRQ_COUNT])(void) = {
    irq32, irq33, irq34, irq35, irq36, irq37, irq38, irq39,
    irq40, irq41, irq42, irq43, irq44, irq45, irq46, irq47,
};

void idt_register_handler(uint8_t vector, void (*handler)(void));

void irq_init(void)
{
    serial_putln("[irq] remapping PIC to vectors 32-47...");

    pic_init(IRQ_VECTOR_BASE);

    for (uint8_t i = 0; i < IRQ_COUNT; i++) {
        idt_register_handler((uint8_t)(IRQ_VECTOR_BASE + i), irq_stub_table[i]);
    }

    keyboard_init();

    /* Unmask IRQ1 (keyboard) only — timer (IRQ0) stays masked for now. */
    pic_clear_mask(IRQ_KEYBOARD);

    serial_putln("[irq] keyboard unmasked — enabling interrupts (STI)");
    __asm__ __volatile__("sti");
}

void irq_handler(registers_t *regs)
{
    uint8_t vector = (uint8_t)regs->int_no;

    if (vector < IRQ_VECTOR_BASE || vector >= IRQ_VECTOR_BASE + IRQ_COUNT) {
        serial_putln("[irq] spurious?");
        return;
    }

    uint8_t irq = (uint8_t)(vector - IRQ_VECTOR_BASE);

    switch (vector) {
    case IRQ_KEYBOARD_VECTOR:
        keyboard_handle_irq();
        break;

    default:
        /* Unexpected IRQ — acknowledge so the PIC does not stay locked. */
        break;
    }

    pic_send_eoi(irq);
}
