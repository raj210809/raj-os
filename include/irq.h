/*
 * irq.h — Hardware interrupt (IRQ) layer on top of the IDT.
 *
 * CPU exceptions use vectors 0–31.  After PIC remapping, hardware IRQs use
 * vectors IRQ_VECTOR_BASE .. IRQ_VECTOR_BASE+15.
 */

#ifndef IRQ_H
#define IRQ_H

#include "idt.h"

/* First IDT vector reserved for hardware IRQ 0 (timer). */
#define IRQ_VECTOR_BASE  32

/* IRQ line numbers on the PIC (not the same as IDT vector). */
#define IRQ_TIMER        0
#define IRQ_KEYBOARD     1

/* IDT vector for IRQ0 (PIT timer) = 32 + 0 = 32 */
#define IRQ_TIMER_VECTOR  (IRQ_VECTOR_BASE + IRQ_TIMER)

/* IDT vector for the keyboard = 32 + 1 = 33 */
#define IRQ_KEYBOARD_VECTOR  (IRQ_VECTOR_BASE + IRQ_KEYBOARD)

/*
 * Remap PIC, install IRQ gates 32–47, init PIT + keyboard, unmask IRQ0/1, STI.
 */
void irq_init(void);

/* Called from irq_common_stub in assembly for every hardware interrupt. */
void irq_handler(registers_t *regs);

#endif /* IRQ_H */
