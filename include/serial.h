/*
 * serial.h — C-callable wrappers for COM1 debug output (see kernel/serial32.inc).
 *
 * Serial is the most reliable way to see kmain() output during bring-up:
 * the QEMU window may stay blank with some GPU/machine combinations until
 * VGA text mode is configured, but -serial stdio always shows these messages.
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

/* Print a null-terminated string to COM1 (UART 0x3F8). */
void serial_puts(const char *str);

/* Same as serial_puts(), then send CR LF. */
void serial_putln(const char *str);

/* Print one character to COM1 (no newline). */
void serial_putchar(char c);

/* Print a 32-bit value as eight hex digits (no newline). */
void serial_print_hex32(uint32_t value);

/* Print a 64-bit value as sixteen hex digits (no newline). */
void serial_print_hex64(uint64_t value);

#endif /* SERIAL_H */
