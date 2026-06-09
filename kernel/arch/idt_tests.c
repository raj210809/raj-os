/*
 * idt_tests.c — Deliberately trigger CPU exceptions to prove the IDT works.
 */

#include "idt.h"
#include "serial.h"

#include <stdint.h>

void idt_test_ud2(void)
{
    serial_putln("[test] executing UD2 (expects vector 6)...");
    __asm__ __volatile__("ud2");
    serial_putln("[test] returned after UD2 handler");
}

void idt_test_divide_by_zero(void)
{
    serial_putln("[test] integer divide by zero (expects vector 0)...");

    volatile uint64_t dividend = 1;
    volatile uint64_t divisor  = 0;
    volatile uint64_t quotient;

    __asm__ __volatile__(
        "xor %%rdx, %%rdx\n"
        "divq %2"
        : "=a"(quotient)
        : "a"(dividend), "rm"(divisor)
        : "rdx");

    (void)quotient;
    serial_putln("[test] divide returned (unexpected)");
}
