/*
 * keyboard.c — PS/2 keyboard input via IRQ 1.
 *
 * Data port 0x60: scancode byte when IRQ fires.
 * Status port 0x64 bit 0: output buffer full (data ready).
 *
 * Scancode set 1: key release sets bit 7 (0x80).  We ignore releases here.
 */

#include "io.h"
#include "keyboard.h"
#include "serial.h"
#include "vga.h"

#define KBD_DATA_PORT    0x60
#define KBD_STATUS_PORT  0x64
#define KBD_STATUS_OBF   0x01

/*
 * US QWERTY scancode set 1 — make code (0x01–0x39) to ASCII.
 * 0 = unmapped / special handled separately.
 */
/* US QWERTY scancode set 1 — indices 0x00–0x39 cover normal keys. */
static const char scancode_ascii[0x3A] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,  '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,  ' ',
};

void keyboard_init(void)
{
    serial_putln("[kbd] PS/2 keyboard ready (IRQ1 -> echo on VGA)");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_write_at(22, 0, "Keyboard (IRQ1): click QEMU window, then type below");
    vga_write_at(24, 0, "> ");
    /* vga_puts above left the cursor mid-screen; route input to the prompt. */
    vga_set_cursor(24, 2);
}

void keyboard_handle_irq(void)
{
    /*
     * Drain every pending scancode.  The PS/2 controller can buffer more than
     * one byte; reading only once per IRQ can leave keys stuck.
     */
    while ((inb(KBD_STATUS_PORT) & KBD_STATUS_OBF) != 0) {
        uint8_t sc = inb(KBD_DATA_PORT);

        /* Key release — high bit set in scancode set 1. */
        if (sc & 0x80U) {
            continue;
        }

        if (sc >= sizeof(scancode_ascii) || sc == 0) {
            continue;
        }

        char c = scancode_ascii[sc];
        if (c == 0) {
            continue;
        }

        vga_putchar((int)c);
    }
}
