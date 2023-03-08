/*
 * devices.h
 *
 *  Created on: Jan 21, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _DEVICES_H_
#define _DEVICES_H_

#include <devices/half-duplex.h>
#include <devices/console.h>
#include <devices/buzzer.h>

struct half_duplex *device_get_half_duplex_channel(uint32_t channel);
struct console *device_get_console(void);
struct buzzer *device_get_buzzer();

#endif
