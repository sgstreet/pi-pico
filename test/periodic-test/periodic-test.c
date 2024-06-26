#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sys/swi.h>
#include <sys/systick.h>
#include <sys/periodic.h>

static volatile unsigned long pticks = 0;

static void periodic_tick(unsigned int swi, void *context)
{
	++pticks;
}

int main(int argc, char **argv)
{
	swi_register(5, INTERRUPT_NORMAL, periodic_tick, 0);
	swi_enable(5);

	periodic_register(1, 5, 8);
	periodic_enable(1);

	while (true) {
		printf("periodic-ticks: %lu\n", pticks);
		systick_delay(1000);
	}
}

