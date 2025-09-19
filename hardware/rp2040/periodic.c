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

#include <hardware/rp2040/timer.h>

#include <sys/periodic.h>

struct periodic_channel
{
	uint32_t period;
	uint32_t next;
	unsigned int swi;
	volatile uint32_t *alarm;
};

void TIMER_IRQ_0_Handler(void);
void TIMER_IRQ_1_Handler(void);
void TIMER_IRQ_2_Handler(void);
void TIMER_IRQ_3_Handler(void);

unsigned long long periodic_timestamp(void);

static struct periodic_channel channels[TIMER_NUM_CHANNELS] = { 0 };

static __fast_section void periodic_alarm(IRQn_Type irq, void *context)
{
	struct periodic_channel *channel = context;

	/* Update the alarm, hopefully we did not miss it */
	channel->next = channel->next + channel->period;
	*channel->alarm = channel->next;

	/* Clear the alarm interrupt */
	clear_bit(&TIMER->INTR, irq - TIMER_IRQ_0_IRQn);

	/* Kick the handler */
	swi_trigger(channel->swi);
}

__fast_section void TIMER_IRQ_0_Handler(void)
{
	/* Forward only if triggered */
	if (TIMER->INTS & TIMER_INTS_ALARM_0_Msk)
		periodic_alarm(TIMER_IRQ_0_IRQn, &channels[0]);
}

__fast_section void TIMER_IRQ_1_Handler(void)
{
	/* Forward only if triggered */
	if (TIMER->INTS & TIMER_INTS_ALARM_1_Msk)
		periodic_alarm(TIMER_IRQ_1_IRQn, &channels[1]);
}

__fast_section void TIMER_IRQ_2_Handler(void)
{
	/* Forward only if triggered */
	if (TIMER->INTS & TIMER_INTS_ALARM_2_Msk)
		periodic_alarm(TIMER_IRQ_2_IRQn, &channels[2]);
}

__fast_section void TIMER_IRQ_3_Handler(void)
{
	/* Forward only if triggered */
	if (TIMER->INTS & TIMER_INTS_ALARM_3_Msk)
		periodic_alarm(TIMER_IRQ_3_IRQn, &channels[3]);
}

unsigned long periodic_ticks(void)
{
	return TIMER->TIMERAWL;
}

int periodic_register(unsigned int channel, unsigned int swi, unsigned int frequency)
{
	assert(channel < TIMER_NUM_CHANNELS || frequency > TIMER_FREQ_HZ);

	/* Make the channel is not running */
	if (TIMER->INTE & (1UL << channel))
		return -EBUSY;

	/* Load the channel */
	channels[channel].period = TIMER_FREQ_HZ / frequency;
	channels[channel].swi = swi;

	/* All good */
	return 0;
}

int periodic_unregister(unsigned int channel)
{
	assert(channel < TIMER_NUM_CHANNELS);

	/* Make the channel is not running */
	if (TIMER->INTE & (1UL << channel))
		return -EBUSY;

	/* Load the channel */
	channels[channel].period = 0;
	channels[channel].swi = 0;

	/* All good */
	return 0;
}

int periodic_enable(unsigned int channel)
{
	assert(channel < TIMER_NUM_CHANNELS);

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

int periodic_disable(unsigned int channel)
{
	assert(channel < TIMER_NUM_CHANNELS);

	/* Enable the interrupt */
	clear_bit(&TIMER->INTE, channel);

	return 0;
}

static void periodic_init(void)
{
	/* Setup the periodic channels */
	for (unsigned int i = 0; i < TIMER_NUM_CHANNELS; ++i) {

		/* Capture the alarm address and mask */
		channels[i].alarm = &TIMER->ALARM0 + i;

		/* Configure alarm interrupt */
		irq_set_priority(TIMER_IRQ_0_IRQn + i, INTERRUPT_REALTIME);
		irq_set_affinity(TIMER_IRQ_0_IRQn + i, i & 0x00000001);

		/* And enable it, it should not fire at the alarm has not been written */
		irq_enable(TIMER_IRQ_0_IRQn + i);
	}
}
PREINIT_PLATFORM_WITH_PRIORITY(periodic_init, HARDWARE_PLATFORM_INIT_PRIORITY);

