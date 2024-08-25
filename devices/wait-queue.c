/*
 * wait-queue.c
 *
 *  Created on: Dec 13, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>

#include <sys/syslog.h>

#include <devices/wait-queue.h>

#define WAIT_QUEUE_WAKE 0x08000000

void wait_queue_ini(struct wait_queue *queue)
{
	assert(queue != 0);

	/* Clear the queue memory */
	memset(queue, 0, sizeof(struct wait_queue));

	/* Initialize the priority sorted list of waiters */
	list_init(&queue->waiters);
}

void wait_queue_fini(struct wait_queue *queue)
{
	assert(queue != 0);

	/* Set the interrupted flag */
	atomic_store(&queue->interrupted, true);

	/* Wake everyone block via interruption, caller responsible for ensure no waiter after cleanup */
	wait_notify(queue, true);

	/* Wait for all the waiters to exit, since the interrupted flag is set, one else will be able to wait */
	while (atomic_load(&queue->count) > 0)
		osDelay(1);
}

struct wait_queue *wait_queue_create(void)
{
	/* Allocate the wait queue */
	struct wait_queue *queue = malloc(sizeof(struct wait_queue));
	if (!queue)
		return 0;

	/* Forward */
	wait_queue_ini(queue);

	/* All good */
	return queue;
}

void wait_queue_destroy(struct wait_queue *queue)
{
	assert(queue != 0);

	/* Forward */
	wait_queue_fini(queue);

	/* Release all memory */
	free(queue);
}

int wait_enqueue(struct wait_queue *queue, unsigned int msec)
{
	struct wait_queue_entry entry;

	assert(queue != 0);

	/* Check for interruptions */
	if (atomic_load(&queue->interrupted)) {
		errno = EINTR;
		return -EINTR;
	}

	/* Initialize the wait queue entry */
	entry.task_id = osThreadGetId();
	if (!entry.task_id) {
		errno = EINVAL;
		return -EINVAL;
	}
	entry.task_priority = osThreadGetPriority(entry.task_id);
	entry.timestamp = osKernelGetTickCount();
	list_init(&entry.node);

	/* Carefully add to the waiters list  */
	unsigned int state = spin_lock_irqsave(&queue->lock);
	list_push(&queue->waiters, &entry.node);
	++queue->count;
	spin_unlock_irqrestore(&queue->lock, state);

	/* Suspend ourselves, error are fatal */
	uint32_t flags = osThreadFlagsWait(WAIT_QUEUE_WAKE, osFlagsWaitAny, msec);
	if ((flags & (osFlagsErrorTimeout | osFlagsErrorResource)) == 0)
		syslog_fatal("failed to suspend task: %d\n", (osStatus_t)flags);

	/* Carefully remove from the waiter list if a timeout */
	state = spin_lock_irqsave(&queue->lock);
	if ((flags & (osFlagsErrorTimeout | osFlagsErrorResource)) != 0) {
		list_remove(&entry.node);
		--queue->count;
	}
	spin_unlock_irqrestore(&queue->lock, state);

	/* Check for interruptions */
	if (atomic_load(&queue->interrupted)) {
		errno = EINTR;
		return -EINTR;
	}

	/* All done */
	return osKernelGetTickCount() - entry.timestamp;
}

int wait_notify(struct wait_queue *queue, bool all)
{
	assert(queue != 0);

	osThreadId_t *wakers = 0;
	size_t num_wakers = 0;

	/* Lock the queue */
	unsigned int state = spin_lock_irqsave(&queue->lock);

	/* Only do wake is there are waiters */
	if (!list_is_empty(&queue->waiters)) {

		/* Build the wake list */
		if (all) {

			/* Add all waiters the vector of wakers */
			wakers = alloca(atomic_load(&queue->count) * sizeof(osThreadId_t));
			struct wait_queue_entry *current;
			list_for_each_entry(current, &queue->waiters, node)
				wakers[num_wakers++] = current->task_id;

		} else {

			/* Find the highest priority waiter to wake, because the list is not empty and the list is locked there must be a least one */
			/* TODO osThreadGetPriority can not be call in interrupt context. This will not find the highest priority thread its priority is changed while blocked on the wait queue */
			wakers = alloca(sizeof(osThreadId_t));
			num_wakers = 1;
			osPriority_t highest_priority = osPriorityNone;
			struct wait_queue_entry *current;
			list_for_each_entry(current, &queue->waiters, node) {
				if (current->task_priority > highest_priority){
					highest_priority = current->task_priority;
					wakers[0] = current->task_id;
				}
			}
		}
	}

	/* Unlock the wait queue */
	spin_unlock_irqrestore(&queue->lock, state);

	/* Got everything in the wake list, resume them */
	for (size_t i = 0; i < num_wakers; ++i) {
		uint32_t flags = osThreadFlagsSet(wakers[i], WAIT_QUEUE_WAKE);
		if (flags & osFlagsError)
			syslog_fatal("failed to wake block task %p: %d\n", wakers[i], (osStatus_t)flags);
	}

	/* Now we are done */
	return num_wakers;
}

void wait_reset(struct wait_queue *queue)
{
	assert(queue != 0);

	/* Set the interrupted flag */
	atomic_store(&queue->interrupted, true);

	/* Wake all waiters */
	wait_notify(queue, true);

	/* Wait for all the waiters to exit, since the interrupted flag is set, one else will be able to wait */
	while (atomic_load(&queue->count) > 0)
		osDelay(1);

	/* Release the wait gate */
	atomic_store(&queue->interrupted, false);
}
