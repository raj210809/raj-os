/*
 * vga.c — VGA text-mode driver (80×25, color buffer at 0xB8000).
 *
 * Hardware background
 * -------------------
 * IBM PC compatibles map the text framebuffer to physical address 0xB8000.
 * In real mode you could use segment 0xB800:0; in protected mode we use a
 * flat pointer because the bootloader installed a flat data segment (0x10).
 *
 * Memory layout per cell (little-endian uint16_t):
 *   [character byte][attribute byte]
 *
 * This file keeps a software cursor (row, column) so vga_putchar can behave
 * like a tiny terminal.  vga_write_at() is for fixed labels (status lines).
 */

#include "vga.h"

/* Pointer to the live framebuffer — volatile because hardware "is" the memory. */
static volatile uint16_t *const VGA_BUFFER =
    (volatile uint16_t *)VGA_PHYS;

/* Cursor position for streaming output (vga_putchar / vga_puts). */
static size_t vga_row;
static size_t vga_column;

/* Attribute byte applied to each newly written character. */
static uint8_t vga_color;

/* Clear one row to spaces with the current attribute. */
static void vga_clear_row(size_t row)
{
    const uint16_t blank = vga_entry(' ', vga_color);

    for (size_t col = 0; col < VGA_WIDTH; col++) {
        VGA_BUFFER[row * VGA_WIDTH + col] = blank;
    }
}

static size_t vga_cell_index(size_t row, size_t col)
{
    if (row >= VGA_HEIGHT) {
        row = VGA_HEIGHT - 1;
    }
    if (col >= VGA_WIDTH) {
        col = VGA_WIDTH - 1;
    }
    return row * VGA_WIDTH + col;
}

/* Scroll every row up by one; bottom row becomes blank. */
static void vga_scroll(void)
{
    for (size_t row = 1; row < VGA_HEIGHT; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            VGA_BUFFER[(row - 1) * VGA_WIDTH + col] =
                VGA_BUFFER[row * VGA_WIDTH + col];
        }
    }

    vga_clear_row(VGA_HEIGHT - 1);
}

void vga_set_color(enum vga_color fg, enum vga_color bg)
{
    vga_color = vga_entry_color(fg, bg);
}

void vga_clear(void)
{
    for (size_t row = 0; row < VGA_HEIGHT; row++) {
        vga_clear_row(row);
    }

    vga_row = 0;
    vga_column = 0;
}

void vga_init(void)
{
    vga_row = 0;
    vga_column = 0;
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_clear();
}

void vga_set_cursor(size_t row, size_t col)
{
    if (row >= VGA_HEIGHT) {
        row = VGA_HEIGHT - 1;
    }
    if (col >= VGA_WIDTH) {
        col = VGA_WIDTH - 1;
    }

    vga_row    = row;
    vga_column = col;
}

void vga_write_at(size_t row, size_t col, const char *str)
{
    if (row >= VGA_HEIGHT) {
        return;
    }

    while (*str != '\0') {
        if (col >= VGA_WIDTH) {
            break;
        }

        VGA_BUFFER[row * VGA_WIDTH + col] =
            vga_entry((unsigned char)*str, vga_color);

        col++;
        str++;
    }
}

int vga_putchar(int c)
{
    if (c == '\n') {
        vga_column = 0;
        if (++vga_row == VGA_HEIGHT) {
            vga_scroll();
            vga_row = VGA_HEIGHT - 1;
        }
        return c;
    }

    if (c == '\r') {
        vga_column = 0;
        return c;
    }

    /* Printable (or other) character — write at the current cursor. */
    VGA_BUFFER[vga_cell_index(vga_row, vga_column)] =
        vga_entry((unsigned char)c, vga_color);

    if (++vga_column == VGA_WIDTH) {
        vga_column = 0;
        if (++vga_row == VGA_HEIGHT) {
            vga_scroll();
            vga_row = VGA_HEIGHT - 1;
        }
    }

    return c;
}

void vga_puts(const char *str)
{
    while (*str != '\0') {
        vga_putchar((int)*str);
        str++;
    }
}
