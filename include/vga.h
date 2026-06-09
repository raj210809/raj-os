/*
 * vga.h — Public API for the VGA text-mode driver.
 *
 * VGA text mode paints the screen by writing 16-bit cells into a fixed
 * region of physical RAM at 0xB8000.  There is no BIOS in protected mode,
 * so every character and color must be written by our code.
 *
 * Each cell is laid out as:
 *   bits  0..7  = ASCII character
 *   bits  8..11 = foreground color (0–15)
 *   bits 12..14 = background color (0–15)
 *   bit      15 = blink (usually left 0)
 */

#ifndef VGA_H
#define VGA_H

#include <stddef.h>
#include <stdint.h>

/* Standard 80×25 text mode geometry. */
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

/* Physical address of the color text buffer (see constants.inc). */
#define VGA_PHYS    0xB8000

/*
 * VGA color palette indices (low 4 bits of the attribute byte).
 * Combine foreground and background with VGA_ENTRY_COLOR().
 */
enum vga_color {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW        = 14,
    VGA_COLOR_WHITE         = 15,
};

/* Pack foreground/background into the attribute byte used in each cell. */
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
    return (uint8_t)(fg | (bg << 4));
}

/* Pack one screen cell: character in the low byte, attribute in the high byte. */
static inline uint16_t vga_entry(unsigned char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

/* Reset cursor, default colors, and clear the screen. */
void vga_init(void);

/* Change the color used for future vga_putchar / vga_puts output. */
void vga_set_color(enum vga_color fg, enum vga_color bg);

/* Fill the entire screen with spaces using the current color. */
void vga_clear(void);

/* Write one character; handles '\n' and '\r'.  Returns the character written. */
int vga_putchar(int c);

/* Write a null-terminated string. */
void vga_puts(const char *str);

/*
 * Write a string at a fixed (row, column), 0-based.
 * Does not move the global cursor used by vga_putchar.
 */
void vga_write_at(size_t row, size_t col, const char *str);

/* Move the streaming cursor (vga_putchar / vga_puts). */
void vga_set_cursor(size_t row, size_t col);

#endif /* VGA_H */
