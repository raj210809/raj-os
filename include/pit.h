/*
 * pit.h — Programmable Interval Timer (IRQ 0 / PIT channel 0).
 */

#ifndef PIT_H
#define PIT_H

#include <stdint.h>

/* Program channel 0 and prepare the tick counter (call before unmasking IRQ0). */
void pit_init(uint32_t frequency_hz);

/* Called from irq_handler on every timer interrupt. */
void pit_handle_irq(void);

/* Monotonic tick count since pit_init(). */
uint64_t pit_get_ticks(void);

/* Refresh the on-screen tick counter (also called periodically from the IRQ). */
void pit_update_display(void);

#endif /* PIT_H */
