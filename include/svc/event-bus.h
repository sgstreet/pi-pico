/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  Copyright: @2024 Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _EVENT_H_
#define _EVENT_H_

#include <config.h>

#ifndef EVENT_BUS_NUM_IDS
#define EVENT_BUS_NUM_IDS 16
#endif

#ifndef EVENT_BUS_NUM_HANDLERS
#define EVENT_BUS_NUM_HANDLERS 32
#endif

#ifndef EVENT_BUS_QUEUE_DEPTH
#define EVENT_BUS_QUEUE_DEPTH 16
#endif

#define EVENT_DIRECT 0x80000000

#include <svc/event-ids.h>

struct event
{
	unsigned int id;
	uintptr_t data;
} __packed;

typedef void (*event_handler_t)(const struct event *event, void *context);

int event_register(unsigned int id, event_handler_t handler, void *context);
int event_unregister(unsigned int id, event_handler_t handler);

int event_post(unsigned int id, uintptr_t data);

#endif
