#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include <cmsis/cmsis.h>

#include <sys/syslog.h>

#include "squarewave.pio.h"

int main(int argc, char **argv)
{
	syslog_info("configuring GPIO14 PAD and function\n");
	PADS_BANK0->GPIO14 &= ~(PADS_BANK0_GPIO25_OD_Msk | PADS_BANK0_GPIO1_PUE_Msk | PADS_BANK0_GPIO1_PDE_Msk);
	PADS_BANK0->GPIO15 |= PADS_BANK0_GPIO14_SLEWFAST_Msk;
	IO_BANK0->GPIO14_CTRL = (7UL << IO_BANK0_GPIO14_CTRL_FUNCSEL_Pos);

	syslog_info("loading pio program\n");
	uint32_t *pio_mem = (uint32_t *)&PIO1->INSTR_MEM0;
	for (unsigned i = 0; i < array_sizeof(squarewave_program_instructions); ++i)
		pio_mem[i] = squarewave_program_instructions[i];

	syslog_info("setting to 25MHZ\n");
	float div = SystemCoreClock / (16.0 * 115200);
	uint32_t div_int = div;
	uint32_t div_frac = (div - div_int) * 256;
	PIO1->SM0_CLKDIV = (div_int << PIO0_SM0_CLKDIV_INT_Pos) | (div_frac << PIO0_SM0_CLKDIV_FRAC_Pos);

	syslog_info("configuring SM0 GPIO14\n");
	PIO1->SM0_PINCTRL = (1UL << PIO0_SM0_PINCTRL_SET_COUNT_Pos) | (14UL << PIO0_SM0_PINCTRL_SET_BASE_Pos);

	syslog_info("setting wrap targets\n");
	PIO1->SM0_EXECCTRL &= ~(PIO0_SM0_EXECCTRL_WRAP_BOTTOM_Msk | PIO0_SM0_EXECCTRL_WRAP_TOP_Msk);
	PIO1->SM0_EXECCTRL |= (squarewave_wrap_target << PIO0_SM0_EXECCTRL_WRAP_BOTTOM_Pos) | (squarewave_wrap << PIO0_SM0_EXECCTRL_WRAP_TOP_Pos);

	syslog_info("starting SM0\n");
	PIO1->CTRL |= (1UL << PIO0_CTRL_SM_ENABLE_Pos);

	/* Left it running */
	return 0;
}

