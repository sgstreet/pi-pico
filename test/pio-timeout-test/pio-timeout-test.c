#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include <cmsis/cmsis.h>
#include <board/board.h>
#include <hal/hal.h>

#include <sys/syslog.h>

#include "pio-timeout.pio.h"

static void channel_interrupt_handler(uint32_t machine, uint32_t source, void *context)
{
	board_led_toggle(1);
}

int main(int argc, char **argv)
{
	struct hal_pio_machine *machine = hal_pio_get_machine(3);

	hal_pio_load_program(3, pio_timeout_program_instructions, array_sizeof(pio_timeout_program_instructions), 0);

	/* Load channel baud rate */
	float div = SystemCoreClock / (16.0 * 115200);
	uint32_t div_int = div;
	uint32_t div_frac = (div - div_int) * 256;
	machine->clkdiv = (div_int << PIO0_SM0_CLKDIV_INT_Pos) | (div_frac << PIO0_SM0_CLKDIV_FRAC_Pos);

	/* RX timeout in bytes x 16 clocks times, remember 10 bits per byte on the wire, manually load this into the state machine Y register */
	*hal_pio_get_tx_fifo(3) = 4 * 160;

	/* Pull from the TX fifo */
	machine->instr = 0x80a0;

	/* Load Y with the the timeout values */
	machine->instr = 0xa047;

	hal_pio_register_irq(3, channel_interrupt_handler, (void *)3);
	hal_pio_enable_irq(3, (1UL << (PIO0_INTR_SM0_Pos + (3 & 0x3))));

	hal_pio_machine_enable(3);

	while (true);

	/* Left it running */
	return 0;
}

