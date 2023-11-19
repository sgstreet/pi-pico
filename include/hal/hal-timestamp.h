/*
 * hal-timestamp.h
 *
 *  Created on: Oct 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _HAL_TIMESTAMP_H_
#define _HAL_TIMESTAMP_H_

unsigned long hal_timestamp_frequency(void);
unsigned long long hal_timestamp_raw(void);
unsigned long hal_timestamp_usec(void);
unsigned long hal_timestamp_msec(void);
float hal_timestamp_float(void);

#endif
