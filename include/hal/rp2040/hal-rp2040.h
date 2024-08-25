#ifndef _HAL_RP2040_H_
#define _HAL_RP2040_H_

#include <stdint.h>
#include <stdbool.h>

#include <cmsis/cmsis.h>

#define HAL_NUM_PIO_MACHINES 8UL
#define HAL_NUM_PERIODIC_CHANNELS 4UL

#define HAL_PERIODIC_TIME_BASE_HZ 1000000

#include <hal/rp2040/hal-rp2040-pwm.h>

#endif
