/*
 * periodic.h
 *
 *  Created on: Jun 25, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _PERIODIC_H_
#define _PERIODIC_H_

#include <sys/swi.h>

unsigned long periodic_frequency(void);
unsigned long periodic_ticks(void);

int periodic_register(unsigned int channel, unsigned int swi, unsigned int frequency);
int periodic_unregister(unsigned int channel);

int periodic_enable(unsigned int channel);
int periodic_disable(unsigned int channel);

#endif
