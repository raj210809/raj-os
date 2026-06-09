/*
 * pic.c — Remap and control the dual 8259 PICs.
 */

#include "io.h"
#include "pic.h"

void pic_init(uint8_t vector_offset)
{
    /*
     * ICW1: start initialization sequence (edge triggered, cascade).
     * ICW2: vector offset for master (IRQ0 -> vector_offset) and slave (+8).
     * ICW3: tell master a slave is on IRQ2; slave's cascade identity.
     * ICW4: 8086 mode.
     */
    outb(PIC_MASTER_CMD, 0x11);
    outb(PIC_SLAVE_CMD,  0x11);

    outb(PIC_MASTER_DATA, vector_offset);
    outb(PIC_SLAVE_DATA,  vector_offset + 8U);

    outb(PIC_MASTER_DATA, 0x04);   /* slave on IRQ2 */
    outb(PIC_SLAVE_DATA,  0x02);   /* slave cascade identity */

    outb(PIC_MASTER_DATA, 0x01);   /* 8086 mode */
    outb(PIC_SLAVE_DATA,  0x01);

    /* Mask every IRQ until drivers explicitly unmask what they need. */
    outb(PIC_MASTER_DATA, 0xFF);
    outb(PIC_SLAVE_DATA,  0xFF);
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8U) {
        outb(PIC_SLAVE_CMD, PIC_EOI);
    }
    outb(PIC_MASTER_CMD, PIC_EOI);
}

void pic_set_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t  line;

    if (irq < 8U) {
        port = PIC_MASTER_DATA;
        line = irq;
    } else {
        port = PIC_SLAVE_DATA;
        line = irq - 8U;
    }

    uint8_t mask = (uint8_t)(inb(port) | (uint8_t)(1U << line));
    outb(port, mask);
}

void pic_clear_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t  line;

    if (irq < 8U) {
        port = PIC_MASTER_DATA;
        line = irq;
    } else {
        port = PIC_SLAVE_DATA;
        line = irq - 8U;
    }

    uint8_t mask = (uint8_t)(inb(port) & (uint8_t)~(1U << line));
    outb(port, mask);
}
