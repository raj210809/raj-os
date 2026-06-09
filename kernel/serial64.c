/*
 * serial64.c — COM1 debug output from C (long mode).
 */

#include "serial.h"

#include <stdint.h>

#define COM1_DATA  0x3F8
#define COM1_LSR   0x3FD
#define LSR_THRE   0x20

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_putchar_raw(char c)
{
    while ((inb(COM1_LSR) & LSR_THRE) == 0) {
    }
    outb(COM1_DATA, (uint8_t)c);
}

void serial_putchar(char c)
{
    serial_putchar_raw(c);
}

void serial_puts(const char *s)
{
    while (*s != '\0') {
        serial_putchar_raw(*s++);
    }
}

void serial_putln(const char *s)
{
    serial_puts(s);
    serial_putchar_raw('\r');
    serial_putchar_raw('\n');
}

void serial_print_hex32(uint32_t value)
{
    for (int i = 7; i >= 0; i--) {
        uint32_t nibble = (value >> (i * 4U)) & 0xFU;
        char c = (char)(nibble < 10U ? ('0' + nibble) : ('A' + nibble - 10U));
        serial_putchar_raw(c);
    }
}

void serial_print_hex64(uint64_t value)
{
    for (int i = 15; i >= 0; i--) {
        uint64_t nibble = (value >> (i * 4U)) & 0xFUL;
        char c = (char)(nibble < 10U ? ('0' + nibble) : ('A' + nibble - 10U));
        serial_putchar_raw(c);
    }
}
