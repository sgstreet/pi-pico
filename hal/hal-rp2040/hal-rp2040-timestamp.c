/*
 * hal-rp2040-timestamp.c
 *
 *  Created on: Oct 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <compiler.h>

#include <hal/hal.h>

unsigned long hal_timestamp_frequency(void)
{
	return HAL_PERIODIC_TIME_BASE_HZ;
}

unsigned long long hal_timestamp_raw(void)
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

unsigned long hal_timestamp_msec(void)
{
	return TIMER->TIMERAWL / (HAL_PERIODIC_TIME_BASE_HZ / 1000);
}

unsigned long hal_timestamp_usec(void)
{
	return TIMER->TIMERAWL;
}

float hal_timestamp_float(void)
{
	return (float)hal_timestamp_raw() / HAL_PERIODIC_TIME_BASE_HZ;
}

