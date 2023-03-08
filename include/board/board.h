#ifndef _BOARD_H_
#define _BOARD_H_

#include <cmsis/cmsis.h>

#include __BOARD

IRQn_Type board_swi_lookup(unsigned int swi);

extern void board_led_on(unsigned int led);
extern void board_led_off(unsigned int led);
extern void board_led_toggle(unsigned int led);

extern int board_getchar(unsigned int msec);
extern int board_putchar(int c);
extern int board_puts(const char *s);

#endif
