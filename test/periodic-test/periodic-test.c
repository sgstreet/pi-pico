#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sys/swi.h>
#include <hal/hal.h>
#include <sys/systick.h>

static volatile unsigned long pticks = 0;

static void periodic_tick(unsigned int swi, void *context)
{
	++pticks;
}

int main(int argc, char **argv)
{
	swi_register(BOARD_SWI_NUM - 1, INTERRUPT_NORMAL, periodic_tick, 0);
	swi_enable(BOARD_SWI_NUM - 1);

	hal_periodic_register(1, BOARD_SWI_NUM - 1, 8);
	hal_periodic_enable(1);

	while (true) {
		printf("periodic-ticks: %lu\n", pticks);
		systick_delay(1000);
	}
}

