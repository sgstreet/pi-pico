/*
 * multicore-event.h
 *
 *  Created on: Jun 19, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _MULTICORE_EVENT_H_
#define _MULTICORE_EVENT_H_

#include <stdint.h>

/* Must be a power of 2 */
#define MULTICORE_NUM_EVENTS 16UL

#define MULTICORE_EVENT_EXECUTE_FLASH 0x10000000
#define MULTICORE_EVENT_EXECUTE_SRAM 0x20000000

typedef void (*multicore_event_handler_t)(uint32_t event, void *context);

void multicore_event_register(uint32_t event_id, multicore_event_handler_t handler, void *context);
void multicore_event_unregister(uint32_t event_id, multicore_event_handler_t handler);

void multicore_event_post(uintptr_t event);

#endif
