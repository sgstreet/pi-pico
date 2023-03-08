/*
 * buzzer.c
 *
 *  Created on: Feb 10, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <board/board.h>
#include <hal/hal.h>

#include <devices/buzzer.h>

#define BUZZER_DIVISOR (255.0F + 15.0F/16.0F)

static void play_timer(void *context)
{
	assert(context != 0);

	buzzer_stop(context);
}

int buzzer_stop(struct buzzer *buzzer)
{
	buzzer->pwm->csr &= ~PWM_CH0_CSR_EN_Msk;

	/* Stop the timer, ignore errors is the time is already expired */
	osStatus_t os_status = osTimerStop(buzzer->timer);
	if (os_status != osOK && os_status != osErrorResource) {
		errno = errno_from_rtos(os_status);
		return -errno;
	}

	/* Should be stopped */
	return 0;
}

int buzzer_play_freq(struct buzzer *buzzer, float freq, unsigned int duration, unsigned int volume)
{
	assert(buzzer != 0);

	/* Do not do anything is the durations is zero */
	if (duration == 0)
		return 0;

	/* Start up the PWM */
	buzzer->pwm->top = (BOARD_CLOCK_SYS_HZ / (freq * BUZZER_DIVISOR)) - 1;;
	buzzer->pwm->cc = buzzer->pwm->top >> 1;
	buzzer->pwm->csr |= PWM_CH0_CSR_EN_Msk;

	/* Is duration is limited start the timer */
	if (duration < osWaitForever) {

		/* Duration is in tick (~1msec) */
		osStatus_t os_status = osTimerStart(buzzer->timer, duration);
		if (os_status != osOK) {
			errno = errno_from_rtos(os_status);
			return -errno;
		}
	}

	/* Should be playing */
	return 0;
}

int buzzer_ini(struct buzzer *buzzer, uint32_t gpio)
{
	assert(buzzer != 0);

	memset(buzzer, 0, sizeof(struct buzzer));

	/* Get the pwm slice using the GPIO number */
	buzzer->pwm = hal_rp2040_pwm_get_slice(gpio);

	/* Use max integer divider, no output and min frequency  */
	buzzer->pwm->div = (255UL << PWM_CH0_DIV_INT_Pos) | (15UL << PWM_CH0_DIV_FRAC_Pos);
	buzzer->pwm->cc = 0;
	buzzer->pwm->top = 0;

	/* Initialize the os timer of note duration */
	buzzer->timer = osTimerNew(play_timer, osTimerOnce, buzzer, 0);
	if (!buzzer->timer)
		return -errno;

	/* All good, should be initialized */
	return 0;
}

void buzzer_fini(struct buzzer *buzzer)
{
}

struct buzzer *buzzer_create(uint32_t gpio)
{
	/* Get some memory */
	struct buzzer *buzzer = malloc(sizeof(struct buzzer));
	if (!buzzer)
		return 0;

	/* Forward to initializer */
	int status = buzzer_ini(buzzer, gpio);
	if (status < 0)
		return 0;

	/* Should be good */
	return buzzer;
}

void buzzer_destroy(struct buzzer* buzzer)
{
	assert(buzzer != 0);

	/* Forward to finalizer */
	buzzer_fini(buzzer);

	/* Do not forget to release the memory */
	free(buzzer);
}
