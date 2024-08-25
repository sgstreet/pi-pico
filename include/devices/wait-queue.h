/*
 * wait-queue.h
 *
 *  Created on: Dec 13, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _WAIT_QUEUE_H_
#define _WAIT_QUEUE_H_

#include <assert.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <linked-list.h>
#include <sys/spinlock.h>

#include <rtos/rtos.h>

struct wait_queue_entry
{
	osThreadId_t task_id;
	osPriority_t task_priority;
	unsigned int timestamp;
	struct linked_list node;
};

struct wait_queue
{
	struct linked_list waiters;
	atomic_uint count;
	atomic_bool interrupted;
	spinlock_t lock;
};

#define wait_event(wq, condition) \
({ \
	__label__ out; \
	int status = 0; \
	do { \
		if (condition) { \
			status = 1; \
			goto out; \
		} \
		status = wait_enqueue(wq, osWaitForever); \
		if (status < 0) \
			goto out; \
	} while (0); \
out: \
	status; \
})

#define wait_event_timeout(wq, condition, msecs) \
({ \
	__label__ out; \
	int status = 0; \
	unsigned int delay = msecs; \
	do { \
		if (condition) { \
			status = delay - status; \
			goto out; \
		} \
		status = wait_enqueue(wq, msecs); \
		if (status < 0) \
			goto out; \
		delay -= status; \
		if (delay <= 0) { \
			status = 0; \
			goto timedout; \
		} \
	} while (0); \
timedout: \
	if (condition) \
		status = 1; \
out: \
	status; \
})

void wait_queue_ini(struct wait_queue *queue);
void wait_queue_fini(struct wait_queue *queue);

struct wait_queue *wait_queue_create(void);
void wait_queue_destroy(struct wait_queue *queue);

int wait_enqueue(struct wait_queue *queue, unsigned int msec);
int wait_notify(struct wait_queue *queue, bool all);
void wait_reset(struct wait_queue *queue);

static inline bool wait_is_busy(const struct wait_queue *queue)
{
	assert(queue != 0);

	return !list_is_empty(&queue->waiters);
}

#endif
