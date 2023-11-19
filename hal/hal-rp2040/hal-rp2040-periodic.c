/*
 * hal-rp2040-periodic.c
 *
 *  Created on: Oct 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdbool.h>
#include <config.h>

#include <init/init-sections.h>
#include <cmsis/cmsis.h>

#include <sys/irq.h>
#include <sys/swi.h>

#include <hal/hal.h>

struct hal_periodic_channel
{
	uint32_t period;
	uint32_t next;
	unsigned int swi;
	volatile uint32_t *alarm;
};

unsigned long long hal_periodic_timestamp(void);

static struct hal_periodic_channel channels[HAL_NUM_PERIODIC_CHANNELS] = { 0 };

static void hal_periodic_alarm(IRQn_Type irq, void *context)
{
	struct hal_periodic_channel *channel = context;

	/* Update the alarm, hopefully we did not miss it */
	channel->next = channel->next + channel->period;
	*channel->alarm = channel->next;

	/* Clear the alarm interrupt */
	clear_bit(&TIMER->INTR, irq - TIMER_IRQ_0_IRQn);

	/* Kick the handler */
	swi_trigger(channel->swi);
}

unsigned long hal_periodic_ticks(void)
{
	return TIMER->TIMERAWL;
}

int hal_periodic_register(unsigned int channel, unsigned int swi, unsigned int frequency)
{
	assert(channel < HAL_NUM_PERIODIC_CHANNELS || frequency > HAL_PERIODIC_TIME_BASE_HZ);

	/* Make the channel is not running */
	if (TIMER->INTE & (1UL << channel))
		return -EBUSY;

	/* Load the channel */
	channels[channel].period = HAL_PERIODIC_TIME_BASE_HZ / frequency;
	channels[channel].swi = swi;

	/* All good */
	return 0;
}

int hal_periodic_unregister(unsigned int channel)
{
	assert(channel < HAL_NUM_PERIODIC_CHANNELS);

	/* Make the channel is not running */
	if (TIMER->INTE & (1UL << channel))
		return -EBUSY;

	/* Load the channel */
	channels[channel].period = 0;
	channels[channel].swi = 0;

	/* All good */
	return 0;
}

int hal_periodic_enable(unsigned int channel)
{
	assert(channel < HAL_NUM_PERIODIC_CHANNELS);

	/* Make the channel is not running */
	if (TIMER->INTE & (1UL << channel))
		return -EBUSY;

	/* Setup the alarm */
	channels[channel].next = TIMER->TIMERAWL + channels[channel].period;
	*channels[channel].alarm = channels[channel].next;

	/* Enable the interrupt */
	set_bit(&TIMER->INTE, channel);

	/* Everything should be running */
	return 0;
}

int hal_periodic_disable(unsigned int channel)
{
	assert(channel < HAL_NUM_PERIODIC_CHANNELS);

	/* Enable the interrupt */
	clear_bit(&TIMER->INTE, channel);

	return 0;
}

static void hal_periodic_init(void)
{
	/* Setup the periodic channels */
	for (unsigned int i = 0; i < HAL_NUM_PERIODIC_CHANNELS; ++i) {

		/* Capture the alarm address */
		channels[i].alarm = &TIMER->ALARM0 + i;

		/* Register the hardware interrupt */
		irq_register(TIMER_IRQ_0_IRQn + i, INTERRUPT_REALTIME, hal_periodic_alarm, &channels[i]);

		/* And enable it, it should not fire at the alarm has not been written */
		irq_enable(TIMER_IRQ_0_IRQn + i);
	}
}
PREINIT_PLATFORM_WITH_PRIORITY(hal_periodic_init, HAL_PLATFORM_PRIORITY);

