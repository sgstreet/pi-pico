/*
 * timestamp.h
 *
 *  Created on: Jun 25, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

unsigned long timestamp_frequency(void);
unsigned long long timestamp(void);
unsigned long timestamp_usec(void);
unsigned long timestamp_msec(void);
float timestamp_float(void);
double timestamp_double(void);

#endif
