#ifndef _HAL_RP2040_H_
#define _HAL_RP2040_H_

#include <stdint.h>
#include <stdbool.h>

#include <cmsis/cmsis.h>

#define HAL_NUM_LOCKS 32UL
#define HAL_NUM_PIO_MACHINES 8UL

#include <hal/rp2040/hal-rp2040-dma.h>
#include <hal/rp2040/hal-rp2040-pio.h>
#include <hal/rp2040/hal-rp2040-pwm.h>
#include <hal/rp2040/hal-rp2040-multicore.h>

#endif
