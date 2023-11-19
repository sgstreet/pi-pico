/*
 * hal-periodic.h
 *
 *  Created on: Oct 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _HAL_PERIODIC_H_
#define _HAL_PERIODIC_H_

unsigned long hal_periodic_ticks(void);

int hal_periodic_register(unsigned int channel, unsigned int swi, unsigned int frequency);
int hal_periodic_unregister(unsigned int channel);

int hal_periodic_enable(unsigned int channel);
int hal_periodic_disable(unsigned int channel);

#endif
