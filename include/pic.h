/*
 * pic.h — 8259 Programmable Interrupt Controller (master + slave).
 *
 * PC hardware IRQs 0–15 are wired to the PIC.  By default they map to CPU
 * vectors 0x08–0x0F, which collide with CPU exceptions.  We remap them to
 * 0x20–0x2F (32–47) before enabling hardware interrupts.
 */

#ifndef PIC_H
#define PIC_H

#include <stdint.h>

#define PIC_MASTER_CMD  0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_CMD   0xA0
#define PIC_SLAVE_DATA  0xA1

#define PIC_EOI         0x20

/* Remap IRQ 0–15 to IDT vectors IRQ_BASE .. IRQ_BASE+15 */
void pic_init(uint8_t vector_offset);

/* Tell the PIC that IRQ `irq` (0–15) has been handled. */
void pic_send_eoi(uint8_t irq);

/* Mask (1) or unmask (0) a single IRQ line. */
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);

#endif /* PIC_H */
