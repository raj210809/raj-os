/*
 * keyboard.h — PS/2 keyboard driver (IRQ 1, scancode set 1).
 *
 * The keyboard does not give ASCII directly — it sends scancodes on port 0x60
 * when IRQ 1 fires.  We translate make codes to characters and echo via VGA.
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_init(void);

/* Handle one IRQ from the keyboard (read port 0x60, echo to screen). */
void keyboard_handle_irq(void);

#endif /* KEYBOARD_H */
