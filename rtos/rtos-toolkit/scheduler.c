/*
 * Copyright (C) 2022 Stephen Street
 *
 * Created on: Feb 13, 2017
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

#include <cmsis/cmsis.h>

#include <sys/systick.h>
#include <sys/svc.h>

#include <rtos/rtos-toolkit/scheduler.h>

#include "debugger.h"

#define ALIGNMENT_ROUND_SIZE(SIZE, BYTES) ((SIZE + (BYTES - 1)) & ~(BYTES - 1))
#define ALIGNMENT_ROUND_TYPE(TYPE, BYTES) ((sizeof(TYPE) + (BYTES - 1)) & ~(BYTES - 1))
#define DELAY_MAX (UINT32_MAX / 2)

#define sched_container_of(ptr, type, member) \
	({ \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
	})

#define sched_container_of_or_null(ptr, type, member) \
	({ \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        __mptr ? (type *)((char *)__mptr - offsetof(type, member)) : 0; \
	})

void scheduler_start_svc(uint32_t svc, struct exception_frame *frame);
void scheduler_create_svc(uint32_t svc, struct exception_frame *frame);
void scheduler_yield_svc(uint32_t svc, struct exception_frame *frame);
void scheduler_terminate_svc(uint32_t svc, struct exception_frame *frame);
void scheduler_suspend_svc(uint32_t svc, struct exception_frame *frame);
void scheduler_resume_svc(uint32_t svc, struct exception_frame *frame);
void scheduler_join_svc(uint32_t svc, struct exception_frame *frame);
void scheduler_wait_svc(uint32_t svc, struct exception_frame *frame);
void scheduler_wake_svc(uint32_t svc, struct exception_frame *frame);
void scheduler_priority_svc(uint32_t svc, struct exception_frame *frame);

struct scheduler_frame *scheduler_switch(struct scheduler_frame *frame);

void SysTick_Handler(void);

extern __weak void scheduler_idle_hook(void);
extern __weak void scheduler_switch_hook(struct task *task);
extern __weak void scheduler_terminated_hook(struct task *task);
extern __weak void scheduler_tick_hook(unsigned long ticks);
extern __weak void scheduler_tls_init_hook(void *tls);

struct scheduler *scheduler = 0;

static inline void sched_list_init(struct sched_list *list)
{
	list->next = list;
	list->prev = list;
}

static inline bool sched_list_empty(struct sched_list *list)
{
	assert(list != 0);

	return list->next == list;
}

static inline void sched_list_insert(struct sched_list *node, struct sched_list *first, struct sched_list *second)
{
	assert(node != 0 && first != 0 && second != 0);

	second->prev = node;
	node->next = second;
	node->prev = first;
	first->next = node;
}

static inline void sched_list_insert_after(struct sched_list *entry, struct sched_list *node)
{
	assert(entry != 0 && node != 0);

	node->next = entry->next;
	node->prev = entry;
	entry->next->prev = node;
	entry->next = node;
}

static inline void sched_list_insert_before(struct sched_list *entry, struct sched_list *node)
{
	assert(entry != 0 && node != 0);

	node->next = entry;
	node->prev = entry->prev;
	entry->prev->next = node;
	entry->prev = node;
}

static inline void sched_list_add(struct sched_list *list, struct sched_list *node)
{
	assert(list != 0 && node != 0);

	node->next = list;
	node->prev = list->prev;
	list->prev->next = node;
	list->prev = node;
}

static inline void sched_list_remove(struct sched_list *node)
{
	assert(node != 0);

	node->next->prev = node->prev;
	node->prev->next = node->next;
	node->next = node;
	node->prev = node;
}

static inline void sched_list_push(struct sched_list *list, struct sched_list *node)
{
	assert(list != 0 && node != 0);

	sched_list_insert(node, list->prev, list);
}

static inline struct sched_list *sched_list_pop(struct sched_list *list)
{
	assert(list != 0);

	struct sched_list *node = list->next;

	if (node == list)
		return 0;

	sched_list_remove(node);

	return node;
}

static inline int sched_list_is_linked(struct sched_list *node)
{
	assert(node != 0);

	return node->next != node;
}

#define sched_list_pop_entry(list, type, member) sched_container_of_or_null(sched_list_pop(list), type, member)
#define sched_list_entry(ptr, type, member) sched_container_of_or_null(ptr, type, member)
#define sched_list_first_entry(list, type, member) sched_list_entry((list)->next, type, member)
#define sched_list_last_entry(list, type, member) sched_list_entry((list)->prev, type, member)
#define sched_list_next_entry(position, member) sched_list_entry((position)->member.next, typeof(*(position)), member)
#define sched_list_prev_entry(position, member) sched_list_entry((position)->member.prev, typeof(*(position)), member)

#define sched_list_for_each(cursor, list) \
	for (cursor = (list)->next; cursor != (list); cursor = cursor->next)

#define sched_list_for_each_mutable(cursor, current, list) \
	for (cursor = (list)->next, current = cursor->next; cursor != (list); cursor = current, current = cursor->next)

#define sched_list_for_each_entry(cursor, list, member) \
	for (cursor = sched_list_first_entry(list, typeof(*cursor), member); &cursor->member != (list); cursor = sched_list_next_entry(cursor, member))

#define sched_list_for_each_entry_mutable(cursor, current, list, member) \
	for (cursor = sched_list_first_entry(list, typeof(*cursor), member), current = sched_list_next_entry(cursor, member); &cursor->member != (list); cursor = current, current = sched_list_next_entry(current, member))

#if 0
#include <diag/diag.h>
static __unused void sched_dump_queue(struct sched_queue *queue)
{
	struct task *entry;
	sched_list_for_each_entry(entry, &queue->tasks, queue_node) {
		assert(entry->marker == SCHEDULER_TASK_MARKER);
		diag_printf("%p - %s\n", entry, entry->name);
	}
}
#endif

static inline void sched_queue_init(struct sched_queue *queue)
{
	assert(queue != 0);
	queue->size = 0;
	sched_list_init(&queue->tasks);
}

static inline bool sched_queue_empty(struct sched_queue *queue)
{
	assert(queue != 0);
	return queue->size == 0;
}

static inline void sched_queue_remove(struct task *task)
{
	assert(task != 0);

	if (task->current_queue) {
		sched_list_remove(&task->queue_node);
		--task->current_queue->size;
		task->current_queue = 0;
	}
}

static inline void sched_queue_push(struct sched_queue *queue, struct task *task)
{
	assert(queue != 0 && task != 0);

	/* Find the insert point */
	struct task *entry;
	sched_list_for_each_entry(entry, &queue->tasks, queue_node)
		if (entry->current_priority > task->current_priority)
			break;

	/* Insert at the correct position, which might be the head */
	sched_list_insert_before(&entry->queue_node, &task->queue_node);
	task->current_queue = queue;
	++queue->size;
}

static inline struct task *sched_queue_pop(struct sched_queue *queue)
{
	assert(queue != 0);

	struct task* task = sched_list_pop_entry(&queue->tasks, struct task, queue_node);
	if (!task)
		return 0;

	--queue->size;
	task->current_queue = 0;

	return task;
}

static inline unsigned long sched_queue_highest_priority(struct sched_queue *queue)
{
	assert(queue != 0);
	if (queue->size == 0)
		return SCHEDULER_NUM_TASK_PRIORITIES;
	return sched_list_first_entry(&queue->tasks, struct task, queue_node)->current_priority;
}

static inline void sched_queue_reprioritize(struct task *task, unsigned long new_priority)
{
	assert(task != 0 && new_priority >= 0 && new_priority < SCHEDULER_NUM_TASK_PRIORITIES);

	task->current_priority = new_priority;

	struct sched_queue *queue = task->current_queue;
	if (queue) {
		sched_queue_remove(task);
		sched_queue_push(queue, task);
	}
}

static inline __always_inline void request_context_switch(void)
{
	/* Pend the pendsv interrupt */
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;

	/* Make sure this is visible, not required but architecturally recommended */
	__DSB();
}

static inline __always_inline bool is_interrupt_context(void)
{
	return __get_IPSR() != 0 || __get_PRIMASK() != 0;
}

static inline __always_inline bool is_svc_context(void)
{
	uint32_t result;

	asm volatile ("mrs %0, ipsr" : "=r" (result));

	return result == 11;
}

static inline __always_inline bool scheduler_check_stack(struct task *task)
{
	return ((task->flags & SCHEDULER_TASK_STACK_CHECK) == 0) || (task->stack_marker[0] == SCHEDULER_STACK_MARKER && task->stack_marker[1] == SCHEDULER_STACK_MARKER);
}

static inline __always_inline __optimize void scheduler_set_counter(volatile counter64_t *counter, unsigned long long value)
{
	unsigned long state = scheduler_enter_critical();
	counter->jiffies = value;
	scheduler_exit_critical(state);
}

static void scheduler_timer_push(struct task *task, uint32_t delay)
{
	assert(task != 0);

	/* Initialize the timer */
	task->timer_expires.jiffies = scheduler_jiffies() + delay;

	/* Find the insert point */
	struct task *entry;
	sched_list_for_each_entry(entry, &scheduler->timers, timer_node)
		if (entry->timer_expires.jiffies > task->timer_expires.jiffies)
			break;

	/* Insert at the correct position, which might be the head */
	sched_list_insert_before(&entry->timer_node, &task->timer_node);

	/* Update the new timer expire */
	scheduler_set_counter(&scheduler->timer_expires, sched_list_first_entry(&scheduler->timers, struct task, timer_node)->timer_expires.jiffies);
}

static void scheduler_timer_remove(struct task *task)
{
	assert(task != 0);

	sched_list_remove(&task->timer_node);

	/* Update the new timer expire */
	if (sched_list_empty(&scheduler->timers))
		scheduler_set_counter(&scheduler->timer_expires, UINT64_MAX);
	else
		scheduler_set_counter(&scheduler->timer_expires, sched_list_first_entry(&scheduler->timers, struct task, timer_node)->timer_expires.jiffies);
}

static struct task *scheduler_timer_pop(void)
{
	/* Is the timer list empty? */
	if (!sched_list_empty(&scheduler->timers)) {

		/* Check for expired timer */
		struct task *task = sched_list_first_entry(&scheduler->timers, struct task, timer_node);
		if (task->timer_expires.jiffies <= scheduler_jiffies()) {
			scheduler_timer_remove(task);
			return task;
		}
	}

	/* No expired timers */
	return 0;
}

unsigned long long scheduler_jiffies(void)
{
	counter64_t value;

	do {
		value.jiffies = scheduler->time.jiffies;
	} while (value.ticks[1] != scheduler->time.ticks[1]);

	return value.jiffies;
}

unsigned long scheduler_ticks(void)
{
	return scheduler->time.ticks[0];
}

static __isr_section __optimize void rtos_tick(void)
{
	/* Someone may have enabled us too early, ignore */
	if (!scheduler)
		return;

	/* Update the jiffies */
	++scheduler->time.jiffies;
	counter64_t timer_expires = scheduler->timer_expires;

	/* Track is we need a context switch */
	bool switch_context = false;

	/* Has the slice expired for the current task */
	if (scheduler->current && atomic_fetch_sub(&scheduler->current->slice_expires, 1) <= 0)
		switch_context = true;

	/* Check for expired timer */
	if (timer_expires.jiffies <= scheduler->time.jiffies)
		switch_context = true;

	/* Do we need to switch */
	if (switch_context)
		request_context_switch();

	/* Pass to the hook */
	scheduler_tick_hook(scheduler->time.ticks[0]);
}
DECLARE_SYSTICK(rtos_tick);

void scheduler_create_svc(uint32_t svc, struct exception_frame *frame)
{
	assert(frame->r0 != 0 && scheduler != 0);

	struct task *task = (struct task *)frame->r0;

	/* Add the task the scheduler list */
	sched_list_push(&scheduler->tasks, &task->scheduler_node);

	/* Add the new task to the ready queue */
	task->state = TASK_READY;
	sched_queue_push(&scheduler->ready_queue, task);

	/* Since we pushed the task onto the ready queue, do a context switch and return the new task */
	if (scheduler_is_running() && task->current_priority < scheduler->current->current_priority)
		request_context_switch();
}
SCHEDULER_DECLARE_SVC(SCHEDULER_CREATE_SVC, scheduler_create_svc);

void scheduler_yield_svc(uint32_t svc, struct exception_frame *frame)
{
	/* Pend the context switch to switch to the next task */
	scheduler->current->slice_expires = INT32_MAX;
	scheduler->current->state = TASK_READY;
	sched_queue_push(&scheduler->ready_queue, scheduler->current);
	request_context_switch();
}
SCHEDULER_DECLARE_SVC(SCHEDULER_YIELD_SVC, scheduler_yield_svc);

void scheduler_suspend_svc(uint32_t svc, struct exception_frame *frame)
{
	struct task *task = (struct task *)frame->r0;
	unsigned long ticks = frame->r1;

	/* Assume every is ok */
	frame->r0 = 0;

	/* Remove task from any blocked queues */
	sched_queue_remove(task);

	/* What kind of suspend was requested? */
	if (ticks < SCHEDULER_WAIT_FOREVER) {

		/* Mark as sleeping */
		task->state = TASK_SLEEPING;

		/* Add ourselves to the sleep queue */
		sched_queue_push(&scheduler->sleep_queue, task);

		/* Add a timer */
		scheduler_timer_push(task, ticks);

	} else {

		/* Mark as suspended */
		task->state = TASK_SUSPENDED;

		/* Add ourselves to the suspend queue */
		sched_queue_push(&scheduler->suspended_queue, task);
	}

	/* Always for to run a context switch, but if we are not the suspending task we need to be on the ready queue */
	if (scheduler->current != task) {
		scheduler->current->state = TASK_READY;
		sched_queue_push(&scheduler->ready_queue, scheduler->current);
	}

	/* For the context switch */
	request_context_switch();
}
SCHEDULER_DECLARE_SVC(SCHEDULER_SUSPEND_SVC, scheduler_suspend_svc);

void scheduler_resume_svc(uint32_t svc, struct exception_frame *frame)
{
	struct task *task = (struct task *)frame->r0;

	/* Verify the current queue is on suspend queue */
	if (task->current_queue != &scheduler->suspended_queue && task->current_queue != &scheduler->sleep_queue) {
		frame->r0 = -EINVAL;
		return;
	}

	/* Might have an associated timer, remove */
	scheduler_timer_remove(task);

	/* Remove it */
	sched_queue_remove(task);

	/* Clear the return so we can indicate no timeout */
	task->psp->r0 = 0;

	/* Mark as ready */
	task->state = TASK_READY;

	/* Push on the ready queue */
	sched_queue_push(&scheduler->ready_queue, task);

	/* Request a context switch */
	request_context_switch();

	/* All good */
	frame->r0 = 0;
}
SCHEDULER_DECLARE_SVC(SCHEDULER_RESUME_SVC, scheduler_resume_svc);

void scheduler_wait_svc(uint32_t svc, struct exception_frame *frame)
{
	struct futex *futex = (struct futex *)frame->r0;
	long expected = (long)frame->r1;
	long value = (futex->flags & SCHEDULER_FUTEX_CONTENTION_TRACKING) ? expected | SCHEDULER_FUTEX_CONTENTION_TRACKING : expected;
	unsigned long ticks = frame->r2;

	/* Should we block? The second clause prevents a wakeup when the futex becomes contended while on the way into the wait */
	if (atomic_compare_exchange_strong(futex->value, &expected, value) || expected == value) {

		/* Add a timeout if requested */
		if (ticks < SCHEDULER_WAIT_FOREVER)
			scheduler_timer_push(scheduler->current, ticks);

		/* Add to the waiter queue */
		scheduler->current->slice_expires = INT32_MAX;
		scheduler->current->state = TASK_BLOCKED;
		sched_queue_push(&futex->waiters, scheduler->current);

		/* Was priority inheritance requested */
		if ((futex->flags & (SCHEDULER_FUTEX_PI | SCHEDULER_FUTEX_OWNER_TRACKING)) == (SCHEDULER_FUTEX_PI | SCHEDULER_FUTEX_OWNER_TRACKING)) {

			/* Extract the owning task */
			struct task *owner = (struct task *)(value & ~SCHEDULER_FUTEX_CONTENTION_TRACKING);

			/* Add the futex to the list of owned, contented futexes */
			if (!sched_list_is_linked(&futex->owned))
				sched_list_add(&owner->owned_futexes, &futex->owned);

			/* Do we need to boost the priority of the futex owner? */
			unsigned long highest_priority = sched_queue_highest_priority(&futex->waiters);
			if (highest_priority < owner->current_priority)
				sched_queue_reprioritize(owner, highest_priority);
		}

	} else {
		/* Add to the ready queue */
		scheduler->current->state = TASK_READY;
		sched_queue_push(&scheduler->ready_queue, scheduler->current);
	}

	/* At this point assume no timeout */
	frame->r0 = 0;

	/* Always save the frame because regardless of outcome a context switch will be done */
	request_context_switch();
}
SCHEDULER_DECLARE_SVC(SCHEDULER_WAIT_SVC, scheduler_wait_svc);

static int scheduler_wake_futex(struct futex *futex, bool all)
{
	int woken = 0;

	/* If a PI futex, adjust priority current owner */
	if ((futex->flags & (SCHEDULER_FUTEX_PI | SCHEDULER_FUTEX_OWNER_TRACKING)) == (SCHEDULER_FUTEX_PI | SCHEDULER_FUTEX_OWNER_TRACKING)) {

		/* Extract the owning task */
		struct task *owner = (struct task *)(*futex->value & ~SCHEDULER_FUTEX_CONTENTION_TRACKING);

		/* Remove the this futex from the owned list */
		sched_list_remove(&futex->owned);

		/* Now find the highest priority of remaining owned PI futexes */
		unsigned long highest_priority = owner->base_priority;
		struct futex *owned;
		sched_list_for_each_entry(owned, &owner->owned_futexes, owned) {
			unsigned long highest_waiter = sched_queue_highest_priority(&owned->waiters);
			if (highest_waiter < highest_priority)
				highest_priority = highest_waiter;
		}

		/* Re-prioritize the task */
		sched_queue_reprioritize(owner, highest_priority);
	}

	/* Wake up the waiters */
	struct task *task;
	while ((task = sched_queue_pop(&futex->waiters)) != 0) {

		if (futex->flags & SCHEDULER_FUTEX_OWNER_TRACKING)
			atomic_exchange(futex->value, (long)task);

		/* Was priority inheritance requested */
		if ((futex->flags & SCHEDULER_FUTEX_PI) && !sched_queue_empty(&futex->waiters)) {

			/* Add the futex to the list of owned, contented futexes */
			sched_list_add(&task->owned_futexes, &futex->owned);

			/* Adjust the priority of the new owner */
			sched_queue_reprioritize(task, sched_queue_highest_priority(&futex->waiters));
		}

		/* Adjust queue */
		scheduler_timer_remove(task);
		task->slice_expires = INT32_MAX;
		task->state = TASK_READY;
		sched_queue_push(&scheduler->ready_queue, task);

		/* Continue waking more tasks? */
		++woken;
		if (!all || (futex->flags & SCHEDULER_FUTEX_OWNER_TRACKING))
			break;
	}

	/* Update the contention tracking if requested */
	if (futex->flags & SCHEDULER_FUTEX_CONTENTION_TRACKING) {
		if (sched_queue_empty(&futex->waiters))
			atomic_fetch_and(futex->value, ~SCHEDULER_FUTEX_CONTENTION_TRACKING);
		else
			atomic_fetch_or(futex->value, SCHEDULER_FUTEX_CONTENTION_TRACKING);
	}

	/* Return the number of woken tasks */
	return woken;
}

void scheduler_wake_svc(uint32_t svc, struct exception_frame *frame)
{
	struct futex *futex = (struct futex *)frame->r0;
	bool all = (bool)frame->r1;

	/* Initialize the number of woken tasks */
	frame->r0 = scheduler_wake_futex(futex, all);

	/* Request a context switch */
	scheduler->current->state = TASK_READY;
	sched_queue_push(&scheduler->ready_queue, scheduler->current);
	request_context_switch();
}
SCHEDULER_DECLARE_SVC(SCHEDULER_WAKE_SVC, scheduler_wake_svc);

__weak void scheduler_terminated_hook(struct task *task)
{
	assert(task != 0);
	if (task->exit_handler)
		task->exit_handler(task);
}

void scheduler_terminate_svc(uint32_t svc, struct exception_frame *frame)
{
	/* Save the return code */
	struct task *task = (struct task *)frame->r0;

	/* Assume every is ok */
	frame->r0 = 0;

	/* Look for the task control block if the tid is 0, other use the current task */
	if (task != 0 && scheduler->current != task) {

		/* Make sure we have the provided task */
		frame->r0 = -ESRCH;
		struct task *entry;
		sched_list_for_each_entry(entry, &scheduler->tasks, scheduler_node)
			if (entry == task) {
				frame->r0 = 0;
				break;
			}

		/* Should we bail? */
		if (frame->r0 == -ESRCH)
			return;

	} else {

		/* Perform an exit on the current task */
		task = scheduler->current;
		scheduler->current = 0;

		/* Funky order but will work we still exit through the bottom */
		request_context_switch();
	}

	/* Clean up the task */
	task->state = TASK_TERMINATED;
	task->current_queue = 0;
	sched_list_remove(&task->timer_node);
	sched_list_remove(&task->queue_node);
	sched_list_remove(&task->scheduler_node);

	/* Forward to the terminate task hook */
	scheduler_terminated_hook(task);
}
SCHEDULER_DECLARE_SVC(SCHEDULER_TERMINATE_SVC, scheduler_terminate_svc);

void scheduler_priority_svc(uint32_t svc, struct exception_frame *frame)
{
	struct task *task = (struct task *)frame->r0;
	unsigned long priority = frame->r1;

	task->base_priority = priority;
//	if (task->base_priority < task->current_priority) Will the cause a priority inheritance problem?????
	sched_queue_reprioritize(task, task->base_priority);

	/* Let the context switcher sort this out */
	request_context_switch();
}
SCHEDULER_DECLARE_SVC(SCHEDULER_PRIORITY_SVC, scheduler_priority_svc);

/* Needs work, but we do not want to hold enter a critical section to do it */
static bool scheduler_is_viable(void)
{
	/* Well look for blocked tasks schedule task list */
	struct task *potential_task;
	sched_list_for_each_entry(potential_task, &scheduler->tasks, scheduler_node)
		if (potential_task->state >= TASK_RUNNING && (potential_task->flags & SCHEDULER_IGNORE_VIABLE) == 0)
			return true;

	/* Not viable, we should be cleanup and shutdown */
	return false;
}

__weak void scheduler_idle_hook(void)
{
	__WFI();
}

struct scheduler_frame *scheduler_switch(struct scheduler_frame *frame)
{
	unsigned long wakeup;
	struct task *expired;
	struct task *next;

	assert(scheduler != 0);

	/* Only push the current task if we have one and the scheduler is not locked */
	if (scheduler->current != 0) {

		/* Should we skip this context switch? */
		if (scheduler->locked < 0)
			return frame;

		/* Always update the scheduler frame */
		scheduler->current->psp = frame;

		/* Force the running task to complete for the processor */
		if (scheduler->current->state == TASK_RUNNING) {
			scheduler->current->state = TASK_READY;
			scheduler->current->slice_expires = scheduler->current->slice_expires <= 0 ? INT32_MAX : scheduler->current->slice_expires;
			sched_queue_push(&scheduler->ready_queue, scheduler->current);
		}
	}

	/* Try to get the next task */
	while (true) {

		/* Check for deferred wake ups */
		for (int i = 0; i < SCHEDULER_MAX_DEFERED_WAKE; ++i) {
			wakeup = atomic_exchange(&scheduler->deferred_wake[i], 0);
			if (wakeup != 0)
				scheduler_wake_futex((struct futex *)((unsigned long)wakeup & ~0x00000001), (unsigned long)wakeup & 0x00000001);
		}

		/* Ready any expired timers */
		while((expired = scheduler_timer_pop()) != 0) {

			/* Remove from any wait queue */
			sched_queue_remove(expired);

			/* Make ready */
			expired->state = TASK_READY;
			expired->psp->r0 = (uint32_t)-ETIMEDOUT;
			expired->slice_expires = INT32_MAX;

			/* Add to the ready queue */
			sched_queue_push(&scheduler->ready_queue, expired);
		}

		/* Try to get highest priority ready task */
		next = sched_queue_pop(&scheduler->ready_queue);
		if (next) {

			/* Is the stack good? */
			if (scheduler_check_stack(next))
				break;

			/* Sadness but evict the task */
			next->state = TASK_TERMINATED;
			next->psp->r0 = (uint32_t)-EFAULT;
			sched_list_remove(&next->scheduler_node);
			scheduler_terminated_hook(next);
		}

		/* If no potential tasks, terminate the scheduler */
		if (!scheduler_is_viable()) {

			/* The syscall will return ok */
			scheduler->initial_frame->r0 = 0;

			/* This will return to the invoker of scheduler_start */
			return scheduler->initial_frame;
		}

		/* Call the idle hook if present */
		scheduler_idle_hook();
	}


	/* Mark the task as running and return its scheduler frame */
	assert(next->state != TASK_OTHER && next->state != TASK_TERMINATED);
	next->state = TASK_RUNNING;
	if (next->slice_expires == INT32_MAX)
		atomic_store(&next->slice_expires, scheduler->slice_duration);

	/* Let everything run, starting the new task */
	scheduler->current = next;
	scheduler_switch_hook(scheduler->current);
	return scheduler->current->psp;
}

struct task *scheduler_create(void *stack, size_t stack_size, const struct task_descriptor *descriptor)
{
	/* We must have a valid descriptor and stack */
	if (!stack || !scheduler || !descriptor) {
		errno = EINVAL;
		return 0;
	}

	/* Initialize the stack for simple stack consumption measurements */
	if (descriptor->flags & SCHEDULER_TASK_STACK_CHECK) {
		unsigned long *pos = stack;
		unsigned long *end = (void *)(stack + stack_size);
		while (pos < end)
			*pos++ = SCHEDULER_STACK_MARKER;
	}

	/* Initialize the task and add to the scheduler task list */
	struct task *task = stack;
	task->marker = SCHEDULER_TASK_MARKER;
	task->tls = (void *)task + ALIGNMENT_ROUND_TYPE(struct task, 8);
	sched_list_init(&task->timer_node);
	sched_list_init(&task->scheduler_node);
	sched_list_init(&task->queue_node);
	sched_list_init(&task->owned_futexes);
	task->slice_expires = INT32_MAX;
	task->timer_expires.jiffies = UINT64_MAX;
	strncpy(task->name, descriptor->name, TASK_NAME_LEN);
	task->base_priority = descriptor->priority;
	task->current_priority = descriptor->priority;
	task->exit_handler = descriptor->exit_handler;
	task->stack_size = stack_size;
	task->flags = descriptor->flags;
	task->context = descriptor->context;

	/* Build the scheduler frame to use the PSP and run in privileged mode */
	task->psp = (struct scheduler_frame *)((((uintptr_t)stack) + stack_size - sizeof(struct scheduler_frame)) & ~7);
	task->psp->exec_return = 0xfffffffd;
	task->psp->control = CONTROL_SPSEL_Msk;
	task->psp->pc = ((uint32_t)descriptor->entry_point & ~0x01UL);
	task->psp->lr = 0;
	task->psp->psr = xPSR_T_Msk;
	task->psp->r0 = (uint32_t)task->context;

#ifdef BUILD_TYPE_DEBUG
	task->psp->r1 = 0xdead0001;
	task->psp->r2 = 0xdead0002;
	task->psp->r3 = 0xdead0003;
	task->psp->r4 = 0xdead0004;
	task->psp->r5 = 0xdead0005;
	task->psp->r6 = 0xdead0006;
	task->psp->r7 = 0xdead0007;
	task->psp->r8 = 0xdead0008;
	task->psp->r9 = 0xdead0009;
	task->psp->r10 = 0xdead000a;
	task->psp->r11 = 0xdead000b;
	task->psp->r12 = 0xdead000c;
#endif

	/* Initialize the TLS block */
	scheduler_tls_init_hook(task->tls);

	/* And now the stack marker */
	task->stack_marker = task->tls + scheduler->tls_size;

	/* Double check the stack marker */
	if (!scheduler_check_stack(task))
		abort();

	/* Ask scheduler to create the new task */
	return (struct task *)svc_call1(SCHEDULER_CREATE_SVC, (uint32_t)task);
}

int scheduler_init(struct scheduler *new_scheduler, size_t tls_size)
{
	/* Make sure some memory was provided */
	if (!new_scheduler || scheduler != 0) {
		errno = EINVAL;
		return -EINVAL;
	}

	/* This pulls in the layout support structures */
	enable_debugger_support();

	/* Initialize the scheduler */
	memset(new_scheduler, 0, sizeof(struct scheduler));
	new_scheduler->marker = SCHEDULER_MARKER;
	new_scheduler->current = 0;
	new_scheduler->initial_frame = 0;
	new_scheduler->slice_duration = SCHEDULER_TIME_SLICE;
	new_scheduler->tls_size = tls_size;
	new_scheduler->locked = 0;
	new_scheduler->time.jiffies = 0;
	new_scheduler->timer_expires.jiffies = UINT64_MAX;
	sched_queue_init(&new_scheduler->ready_queue);
	sched_queue_init(&new_scheduler->suspended_queue);
	sched_queue_init(&new_scheduler->sleep_queue);
	sched_list_init(&new_scheduler->timers);
	sched_list_init(&new_scheduler->tasks);

	/* Initialize the scheduler exception priorities */
	NVIC_SetPriority(PendSV_IRQn, SCHEDULER_PENDSV_PRIORITY);
	NVIC_SetPriority(SVCall_IRQn, SCHEDULER_SVC_PRIORITY);

	/* Save a scheduler singleton */
	scheduler = new_scheduler;

	/* All good */
	return 0;
}

int scheduler_run(void)
{
	/* Make sure the scheduler has beed initializied */
	if (!scheduler) {
		errno = EINVAL;
		return -EINVAL;
	}

	/* We only return from this when the scheduler is shutdown */
	int result = svc_call0(SCHEDULER_START_SVC);
	if (result < 0) {
		errno = -result;
		return result;
	}

	/* Clear the scheduler singleton, to restart reinitialize */
	scheduler = 0;

	/* All done */
	return 0;
}

bool scheduler_is_running(void)
{
	/* We are running if the initial frame has be set by the start service call */
	return scheduler != 0 && scheduler->initial_frame != 0;
}

int scheduler_lock(void)
{
	assert(scheduler_is_running());

	/* Yes this implements atomic_fetch_sub */
	return scheduler->locked--;
}

int scheduler_unlock(void)
{
	assert(scheduler_is_running());

	/* Yes this implements atomic_fetch_add */
	return scheduler->locked++;
}

int scheduler_lock_restore(int state)
{
	assert(scheduler_is_running());

	return atomic_exchange(&scheduler->locked, state);
}

bool scheduler_is_locked(void)
{
	assert(scheduler_is_running());
	return scheduler->locked < 0;
}

void scheduler_yield(void)
{
	/* Ignore is scheduler is locked */
	if (scheduler->locked < 0)
		return;

	/* We need a service call so that we appear to resume on return from this call */
	(void)svc_call0(SCHEDULER_YIELD_SVC);
}

struct task *scheduler_task(void)
{
	/* We are the current running task */
	return scheduler != 0 ? scheduler->current : 0;
}

int scheduler_suspend(struct task *task)
{
	/* Are we suspending ourselfs? */
	if (!task)
		task = scheduler_task();

	/* We are suspending ourselves, then we need a scheduling frame from the service infrastructure */
	int result = svc_call2(SCHEDULER_SUSPEND_SVC, (uint32_t)task, SCHEDULER_WAIT_FOREVER);
	if (result < 0) {
		errno = -result;
		return -result;
	}

	/* All good */
	return 0;
}

int scheduler_sleep(unsigned long ticks)
{
	/* Msecs should be greater 0, just yield otherwise */
	if (ticks == 0) {
		scheduler_yield();
		return 0;
	}

	/* We are timed suspending ourselves, then we need a scheduling frame from the service infrastructure */
	int result = svc_call2(SCHEDULER_SUSPEND_SVC, (uint32_t)scheduler_task(), ticks);
	if (result < 0 && result != -ETIMEDOUT) {
		errno = -result;
		return result;
	}

	/* All good */
	return 0;
}

int scheduler_resume(struct task *task)
{
	assert(task != 0);

	/* Make the task ready to run */
	int result = svc_call1(SCHEDULER_RESUME_SVC, (uint32_t)task);
	if (result < 0) {
		errno = -result;
		return result;
	}

	/* All good */
	return 0;
}

int scheduler_terminate(struct task *task)
{
	/* Forward */
	int status = svc_call1(SCHEDULER_TERMINATE_SVC, (uint32_t)task);
	if (status < 0)
		errno = -status;
	return status;
}

void scheduler_futex_init(struct futex *futex, long *value, unsigned long flags)
{
	assert(futex != 0);

	futex->marker = SCHEDULER_FUTEX_MARKER;
	futex->value = value;
	futex->flags = flags;
	sched_queue_init(&futex->waiters);
	sched_list_init(&futex->owned);
}

int scheduler_futex_wait(struct futex *futex, long value, unsigned long ticks)
{
	assert(futex != 0);

	int result = svc_call3(SCHEDULER_WAIT_SVC, (uint32_t)futex, value, ticks);
	if (result < 0) {
		errno = -result;
		return result;
	}

	return result;
}

int scheduler_futex_wake(struct futex *futex, bool all)
{
	assert(futex != 0);

	/* Do it the hard way? */
	if (is_interrupt_context()) {

		/* Make the contention tracking and priority inheritance is disabled */
		if ((futex->flags & (SCHEDULER_FUTEX_PI || SCHEDULER_FUTEX_OWNER_TRACKING)) != 0) {
			errno = EINVAL;
			return -EINVAL;
		}

		/* Find an empty slot */
		unsigned long expected = 0;
		unsigned long wakeup = (unsigned long)futex | all;
		for (int i = 0; i < SCHEDULER_MAX_DEFERED_WAKE; ++i) {
			if (atomic_compare_exchange_strong(&scheduler->deferred_wake[i], &expected, wakeup) || expected == wakeup) {
				request_context_switch();
				return 0;
			}
			expected = 0;
		}

		/* Very bad, resource exhausted */
		errno = ENOSPC;
		return -ENOSPC;

	} else {

		/* Send to the wake service */
		int result = svc_call2(SCHEDULER_WAKE_SVC, (uint32_t)futex, all);
		if (result < 0) {
			errno = -result;
			return result;
		}
	}

	return 0;
}

int scheduler_set_priority(struct task *task, unsigned long priority)
{
	/* Range check the new priority */
	if (priority > SCHEDULER_MIN_TASK_PRIORITY) {
		errno = -EINVAL;
		return -EINVAL;
	}

	/* Use the current task if needed */
	if (!task)
		task = scheduler_task();

	/* Forward to the service handler */
	return svc_call2(SCHEDULER_PRIORITY_SVC, (uint32_t)task, priority);
}

unsigned long scheduler_get_priority(struct task *task)
{
	/* Use the current task if needed */
	if (!task)
		task = scheduler_task();

	return task->current_priority;
}

void scheduler_set_flags(struct task *task, unsigned long mask)
{
	/* Use the current task if needed */
	if (!task)
		task = scheduler_task();

	task->flags |= mask;
}

void scheduler_clear_flags(struct task *task, unsigned long mask)
{
	/* Use the current task if needed */
	if (!task)
		task = scheduler_task();

	task->flags &= ~mask;
}

unsigned long scheduler_get_flags(struct task *task)
{
	/* Use the current task if needed */
	if (!task)
		task = scheduler_task();

	return task->flags;
}

void scheduler_for_each(struct sched_list *list, for_each_sched_node_t func, void *context)
{
	scheduler_lock();
	struct sched_list *current;
	sched_list_for_each(current, list)
		if (!func(current, context))
			break;
	scheduler_unlock();
}

