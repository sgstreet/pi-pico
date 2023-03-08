/*
 * hal-pwm.h
 *
 *  Created on: Feb 10, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _HAL_RP2040_PWM_H_
#define _HAL_RP2040_PWM_H_

#include <cmsis/rp2040.h>

struct pwm_slice
{
	__IOM uint32_t csr;
	__IOM uint32_t div;
	__IOM uint32_t ctr;
	__IOM uint32_t cc;
	__IOM uint32_t top;
};

static inline struct pwm_slice *hal_rp2040_pwm_get_slice(uint32_t gpio)
{
   return (struct pwm_slice *)(PWM_BASE + sizeof(struct pwm_slice) * ((gpio >> 1UL) & 7UL));
}

#endif
