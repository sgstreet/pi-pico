/*
 * event.c
 *
 *  Created on: Oct 22, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <config.h>
#include <linked-list.h>

#include <sys/spinlock.h>
#include <sys/syslog.h>

#include <rtos/rtos.h>

#include <svc/svc.h>
#include <svc/event-bus.h>

struct event_dispatch
{
	event_handler_t handler;
	void *context;
};

struct event_handler
{
	struct event_dispatch dispatch;
	struct linked_list node;
};

struct event_bus
{
	osThreadId_t event_dispatch_task_id;
	osMemoryPoolId_t event_handler_pool;
	osDequeId_t event_queue;
	spinlock_t lock;
	unsigned char num_handlers[EVENT_BUS_NUM_IDS];
	struct linked_list handlers[EVENT_BUS_NUM_IDS];
};

static struct event_bus *event_bus = 0;

static void event_bus_dispatch(struct event_bus *bus, const struct event *event)
{
	assert(bus != 0 && event != 0);

	unsigned int id = event->id & ~EVENT_DIRECT;

	/* Build list to dispatch */
	uint32_t state = spin_lock_irqsave(&bus->lock);
	unsigned int count = bus->num_handlers[id];
	struct event_dispatch dispatch[count];
	struct event_handler *entry;
	size_t i = 0;
	list_for_each_entry(entry, &bus->handlers[id], node)
		dispatch[i++] = entry->dispatch;
	spin_unlock_irqrestore(&bus->lock, state);

	/* Now dispatch */
	for (i = 0; i < count; ++count)
		dispatch[i].handler(event, dispatch[i].context);
}

static void event_bus_dispatch_task(void *context)
{
	assert(context != 0);

	struct event event;
	osStatus_t os_status;
	struct event_bus *bus = context;

	/* Loop dispatching events */
	while ((os_status = osDequeGetFront(bus->event_queue, &event, osWaitForever)) == osOK)
		event_bus_dispatch(bus, &event);
}

int event_register(unsigned int id, event_handler_t handler, void *context)
{
	assert(event_bus != 0 && (id & ~EVENT_DIRECT) < EVENT_BUS_NUM_IDS && handler != 0);

	/* Make we do not overrun the count */
	if (event_bus->num_handlers[id & ~EVENT_DIRECT] > EVENT_BUS_NUM_HANDLERS) {
		errno = ENOSPC;
		return -errno;
	}

	/* Allocate a handler */
	struct event_handler *event_handler = osMemoryPoolAlloc(event_bus->event_handler_pool, 0);
	if (!handler) {
		errno = ENOMEM;
		return -errno;
	}

	/* Initialize the handler */
	event_handler->dispatch.handler = handler;
	event_handler->dispatch.context = context;
	list_init(&event_handler->node);

	/* Now carefully add to the handler list */
	uint32_t state = spin_lock_irqsave(&event_bus->lock);
	++event_bus->num_handlers[id & !EVENT_DIRECT];
	list_add(&event_bus->handlers[id & !EVENT_DIRECT], &event_handler->node);
	spin_unlock_irqrestore(&event_bus->lock, state);

	/* All good */
	return 0;
}

int event_unregister(unsigned int id, event_handler_t handler)
{
	assert(event_bus != 0 && (id & ~EVENT_DIRECT) < EVENT_BUS_NUM_IDS && handler != 0);

	struct event_handler *victim = 0;

	/* Carefully find and remove the matching handler */
	uint32_t state = spin_lock_irqsave(&event_bus->lock);
	struct event_handler *entry;
	list_for_each_entry(entry, &event_bus->handlers[id & ~EVENT_DIRECT], node)
		if (entry->dispatch.handler == handler) {
			list_remove(&entry->node);
			--event_bus->num_handlers[id & ~EVENT_DIRECT];
			victim = entry;
			break;
		}
	spin_unlock_irqrestore(&event_bus->lock, state);

	/* Did we find it? */
	if (!victim) {
		/* Humm we did not find it */
		errno = ENOENT;
		return -errno;
	}

	/* Free the handler wrapper */
	osStatus_t os_status = osMemoryPoolFree(event_bus->event_handler_pool, victim);
	if (os_status != osOK) {
		syslog_error("failed to free the event handler: %d\n", os_status);
		errno = errno_from_rtos(os_status);
		return -errno;
	}

	/* All good */
	return 0;
}

int event_post(unsigned int id, uintptr_t data)
{
	assert(event_bus != 0 && (id & ~EVENT_DIRECT) < EVENT_BUS_NUM_IDS);

	struct event event = { .id = id, .data = data };

	/* Direct dispatch? */
	if (id & EVENT_DIRECT) {
		event_bus_dispatch(event_bus, &event);
		return 0;
	}

	/* Post the to the dispatch thread */
	osStatus_t os_status = osDequePutBack(event_bus->event_queue, &event, 0);
	if (os_status != osOK) {
		errno = errno_from_rtos(os_status);
		return -errno;
	}

	/* Everything worked */
	return 0;
}

static __constructor_priority(SERVICES_EVENT_BUS_PRIORITY) void event_bus_ini(void)
{
	/* Allocate the event bus */
	event_bus = calloc(1, sizeof(struct event_bus));
	if (!event_bus)
		syslog_fatal("could not allocate the event bus\n");

	/* Initialize the handler lists */
	for (size_t i = 0; i < EVENT_BUS_NUM_IDS; ++i)
		list_init(&event_bus->handlers[i]);

	/* Create the event handler memory pool */
	osMemoryPoolAttr_t pool_attr = { .name = "event-bus-handler" };
	event_bus->event_handler_pool = osMemoryPoolNew(EVENT_BUS_NUM_HANDLERS, sizeof(struct event_handler), &pool_attr);
	if (!event_bus->event_handler_pool)
		syslog_fatal("could not allocate the event bus handler pool: %d\n", -errno);

	/* Allocate the event queue */
	osDequeAttr_t deque_attr = { .name = "event-bus-queue" };
	event_bus->event_queue = osDequeNew(EVENT_BUS_QUEUE_DEPTH, sizeof(struct event), &deque_attr);
	if (!event_bus->event_queue)
		syslog_fatal("could not create the event bus queue: %d\n", -errno);

	/* Create the dispatch task */
	osThreadAttr_t thread_attr = { .name = "event-bus-dispatch-task", .attr_bits = osThreadJoinable, .stack_size = RTOS_DEFAULT_STACK_SIZE + sizeof(struct event_dispatch) * EVENT_BUS_NUM_HANDLERS, .priority = osPriorityAboveNormal };
	event_bus->event_dispatch_task_id = osThreadNew(event_bus_dispatch_task, event_bus, &thread_attr);
	if (!event_bus->event_dispatch_task_id)
		syslog_fatal("could not create event bus dispatch task: %d\n", -errno);

	syslog_info("ready\n");
}

static __destructor_priority(SERVICES_EVENT_BUS_PRIORITY) void event_bus_fini(void)
{
	assert(event_bus != 0);

	/* Reset the queue to force the dispatch task to exit */
	osStatus_t os_status = osDequeReset(event_bus->event_queue);
	if (os_status != osOK)
		syslog_fatal("failed to reset the event queue: %d\n", os_status);

	/* Join with the terminating task */
	os_status = osThreadJoin(event_bus->event_dispatch_task_id);
	if (os_status != osOK)
		syslog_fatal("failed joint with the event bus dispatch task: %d\n", os_status);

	/* Destroy the event queue */
	os_status = osDequeDelete(event_bus->event_queue);
	if (os_status != osOK)
		syslog_fatal("failed to destroy the event queue: %d\n", os_status);

	/* Release all of the handlers */
	for (size_t i = 0; i < EVENT_BUS_NUM_IDS; ++i) {
		struct event_handler *entry;
		list_for_each_entry(entry, &event_bus->handlers[i], node) {
			os_status = osMemoryPoolFree(event_bus->event_handler_pool, entry);
			if (os_status != osOK)
				syslog_fatal("failed to free event handler: %d\n", os_status);
			--event_bus->num_handlers[i];
		}
	}

	/* Destroy the handler pool */
	os_status = osMemoryPoolDelete(event_bus->event_handler_pool);
	if (os_status != osOK)
		syslog_fatal("failed to destroy the event handler pool: %d\n", os_status);

	/* Almost there, delete the event bus */
	free(event_bus);
	event_bus = 0;

	syslog_info("destroyed");
}
