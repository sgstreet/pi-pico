#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <board/board.h>

#include <config.h>
#include <sys/swi.h>
#include <sys/systick.h>

#define DELAY_MASK 0xffe0
#define TRIGGER_MASK 0xfff0

static uint16_t up_state = 0;
static uint16_t down_state = 0;
static unsigned int value = 0;
static volatile bool state = false;
static volatile bool run = false;

static void debounce(void)
{
	if (!run)
		return;

	value = (SIO->GPIO_IN & (1UL << 15)) != 0;
	up_state = (up_state << 1) | !value | DELAY_MASK;
	down_state = (down_state << 1) | value | DELAY_MASK;

	bool new_state = up_state == TRIGGER_MASK;
	if (new_state ^ state) {
		state = new_state;
		swi_trigger(0);
	}
}
DECLARE_SYSTICK(debounce);

static void debounce_handler(unsigned int swi, void *context)
{
	if (!state)
		board_led_toggle(0);
}

int main(int argc, char **argv)
{
	/* Setup the switch pad with a pull up */
	PADS_BANK0->GPIO15 &= ~(PADS_BANK0_GPIO15_OD_Msk |PADS_BANK0_GPIO15_PDE_Msk);
	PADS_BANK0->GPIO15 |= PADS_BANK0_GPIO15_IE_Msk | PADS_BANK0_GPIO15_PUE_Msk;
	IO_BANK0->GPIO15_CTRL = (5UL << IO_BANK0_GPIO15_CTRL_FUNCSEL_Pos);

	/* Register SWI handler */
	swi_register(0, INTERRUPT_BELOW_NORMAL, debounce_handler, 0);

	run = true;
	while (run) {
		printf("%lu %u up_state: 0x%4hx down_state: 0x%04hx\n", systick_get_ticks(), value, up_state, down_state);
		systick_delay(250);
	}
}

