/*
 * timespace.c
 *
 *  Created on: Jun 25, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include <sys/timestamp.h>

#include <cmsis/cmsis.h>

#include <hardware/rp2040/timer.h>

unsigned long timestamp_frequency(void)
{
	return TIMER_FREQ_HZ;
}

unsigned long long timestamp(void)
{
	union {
		uint32_t parts[2];
		uint64_t val;
	} timestamp;

	/* Load the 64bit value carefully */
	do {
		timestamp.parts[0] = TIMER->TIMERAWL;
		timestamp.parts[1] = TIMER->TIMERAWH;
	} while (timestamp.parts[1] != TIMER->TIMERAWH);

	/* We should have a consistent value */
	return timestamp.val;
}

unsigned long timestamp_msec(void)
{
	return timestamp_usec() / (TIMER_FREQ_HZ / 1000);
}

unsigned long timestamp_usec(void)
{
	return TIMER->TIMERAWL;
}

float timestamp_float(void)
{
	return (float)timestamp() / TIMER_FREQ_HZ;
}

double timestamp_double(void)
{
	return (double)timestamp() / TIMER_FREQ_HZ;
}
