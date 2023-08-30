/*
 * multicore.c
 *
 *  Created on: Apr 26, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sys/delay.h>
#include <sys/syslog.h>
#include <sys/fault.h>
#include <sys/systick.h>
#include <sys/swi.h>
#include <sys/irq.h>

#include <cmsis/cmsis.h>
#include <diag/diag.h>
#include <board/board.h>
#include <rtos/rtos.h>
#include <hal/hal.h>

#include <svc/shell.h>

#define DEBOUNCE_SWI (BOARD_SWI_NUM - 1)
#define DELAY_MASK 0xffe0
#define TRIGGER_MASK 0xfff0

osThreadId_t main_id = 0;

static volatile bool state = false;
static volatile bool blink = true;
static uint16_t up_state = 0;
static uint16_t down_state = 0;

static __unused void debounce(void *context)
{
	uint16_t value = (SIO->GPIO_IN & (1UL << 15)) != 0;
	up_state = (up_state << 1) | !value | DELAY_MASK;
	down_state = (down_state << 1) | value | DELAY_MASK;

	bool new_state = down_state == TRIGGER_MASK;
	if (new_state ^ state) {
		state = new_state;
		swi_trigger(DEBOUNCE_SWI);
	}
}

static void debounce_handler(unsigned int swi, void *context)
{
	if (!state)
		hal_multicore_post_event(1, 0x00000010);
}

static void multicore_blink(uint32_t events, void *context)
{
	blink ^= 1;
}

static int core_test_func(int argc, char **argv)
{
	hal_multicore_register_handler(multicore_blink, 0x00000010, 0);

	while (true) {
		if (blink) {
			board_led_toggle(0);
			delay_cycles(SystemCoreClock / 8);
		}
	}

	return 0;
}

static int exit_cmd(int argc, char **argv)
{
	assert(argc >= 1 && argv != 0);

	if (!main_id) {
		fprintf(stderr, "main thread id is 0\n");
		return EXIT_FAILURE;
	}

	uint32_t flags = osThreadFlagsSet(main_id, 0x7fffffff);
	if (flags & osFlagsError)
		syslog_fatal("error setting main thread flags: %d\n", (osStatus_t)flags);

	return EXIT_SUCCESS;
}

static __shell_command const struct shell_command _exit_cmd =
{
	.name = "exit",
	.usage = "[value]",
	.func = exit_cmd,
};

void save_fault(const struct cortexm_fault *fault, const struct backtrace *entries, int count)
{
	for (size_t i = 0; i < count; ++i)
		diag_printf("%s@%p - %p\n", entries[i].name, entries[i].function, entries[i].address);
}

__noreturn void __assert_fail(const char *expr)
{
	diag_printf("assert failed: %s\n", expr);
	__builtin_trap();
	abort();
}

int main(int argc, char **argv)
{
	/* Setup the switch pad with a pull up */
	PADS_BANK0->GPIO15 &= ~(PADS_BANK0_GPIO15_OD_Msk | PADS_BANK0_GPIO15_PDE_Msk);
	PADS_BANK0->GPIO15 |= PADS_BANK0_GPIO15_IE_Msk | PADS_BANK0_GPIO15_PUE_Msk;
	IO_BANK0->GPIO15_CTRL = (5UL << IO_BANK0_GPIO15_CTRL_FUNCSEL_Pos);

	/* Start running the debouncer */
	systick_register_handler(debounce, 0);

	/* Register SWI handler */
	swi_register(DEBOUNCE_SWI, INTERRUPT_NORMAL, debounce_handler, 0);

	struct hal_multicore_executable core_test = { .argc = 0, .argv = 0, .entry_point = core_test_func };
	hal_multicore_start(1, &core_test);

	hal_multicore_register_handler(multicore_blink, 0x00000010, 0);

	/* Wait for exit */
	syslog_info("waiting for exit\n");
	main_id = osThreadGetId();
	osThreadFlagsWait(0x7fffffff, osFlagsWaitAny, osWaitForever);
	syslog_info("main exiting\n");

	return EXIT_SUCCESS;
}
