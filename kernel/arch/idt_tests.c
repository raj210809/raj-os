/*
 * idt_tests.c — Deliberately trigger CPU exceptions to prove the IDT works.
 */

#include "idt.h"
#include "serial.h"

void idt_test_ud2(void)
{
    serial_putln("[test] executing UD2 (expects vector 6)...");
    __asm__ __volatile__("ud2");
    serial_putln("[test] returned after UD2 handler");
}

void idt_test_divide_by_zero(void)
{
    serial_putln("[test] integer divide by zero (expects vector 0)...");

    /*
     * volatile + inline asm DIV so the compiler cannot optimize the fault away.
     */
    volatile uint32_t dividend = 1;
    volatile uint32_t divisor  = 0;
    volatile uint32_t quotient;

    __asm__ __volatile__(
        "xor %%edx, %%edx\n"
        "divl %2"
        : "=a"(quotient)
        : "a"(dividend), "rm"(divisor)
        : "edx");

    (void)quotient;
    serial_putln("[test] divide returned (unexpected)");
}
