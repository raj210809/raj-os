/*
 * pit.c — PIT channel 0 at ~100 Hz for IRQ 0 (system timer).
 */

#include "io.h"
#include "irq.h"
#include "pit.h"
#include "serial.h"
#include "vga.h"

#define PIT_CMD_PORT     0x43
#define PIT_CH0_PORT     0x40
#define PIT_BASE_HZ      1193182U

static volatile uint64_t timer_ticks;

static void u64_to_dec(uint64_t value, char *buf, size_t bufsz)
{
    size_t i = bufsz - 1U;

    buf[i] = '\0';
    if (value == 0U) {
        buf[--i] = '0';
    } else {
        while (value > 0U && i > 0U) {
            buf[--i] = (char)('0' + (value % 10U));
            value /= 10U;
        }
    }

    /* Shift decimal string to start of buffer. */
    for (size_t j = 0; buf[i + j] != '\0'; j++) {
        buf[j] = buf[i + j];
    }
}

void pit_init(uint32_t frequency_hz)
{
    if (frequency_hz == 0U) {
        frequency_hz = 100U;
    }

    timer_ticks = 0;

    uint32_t divisor = PIT_BASE_HZ / frequency_hz;

    /*
     * 0x36 = channel 0, access lobyte/hibyte, mode 3 (square wave).
     */
    outb(PIT_CMD_PORT, 0x36);
    outb(PIT_CH0_PORT, (uint8_t)(divisor & 0xFFU));
    outb(PIT_CH0_PORT, (uint8_t)((divisor >> 8) & 0xFFU));

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_at(11, 0, "Timer (IRQ0): waiting for ticks...              ");

    serial_puts("[pit] channel 0 @ ");
    {
        char hzbuf[16];
        u64_to_dec(frequency_hz, hzbuf, sizeof(hzbuf));
        serial_puts(hzbuf);
    }
    serial_putln(" Hz (IRQ0)");
}

void pit_handle_irq(void)
{
    timer_ticks++;

    if ((timer_ticks % 10U) == 0U) {
        pit_update_display();
    }

    if ((timer_ticks % 100U) == 0U) {
        serial_puts("[timer] ticks=");
        char buf[24];
        u64_to_dec(timer_ticks, buf, sizeof(buf));
        serial_puts(buf);
        serial_putln("");
    }
}

uint64_t pit_get_ticks(void)
{
    return timer_ticks;
}

void pit_update_display(void)
{
    char buf[24];

    u64_to_dec(timer_ticks, buf, sizeof(buf));
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_at(11, 0, "Timer (IRQ0) ticks: ");
    vga_write_at(11, 20, buf);
    vga_write_at(11, 40, "  (@ ~100 Hz)       ");
}
