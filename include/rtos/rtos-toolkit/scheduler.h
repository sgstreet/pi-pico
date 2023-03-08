/*
 * Copyright (C) 2022 Stephen Street
 *
 * preemt-sched.h
 *
 * Created on: Feb 13, 2017
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */


#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdbool.h>

#include <config.h>
#include <compiler.h>
#include <container-of.h>

#include <cmsis/cmsis.h>

#define TASK_NAME_LEN 32

#define SCHEDULER_SVC_CODE(SVC_CODE) SVC_Handler_ ## SVC_CODE
#define SCHEDULER_DECLARE_SVC(SVC_CODE, SVC_FUNC) void SCHEDULER_SVC_CODE(SVC_CODE)(uint32_t,void*) __attribute__ ((alias(#SVC_FUNC)));

#define SCHEDULER_START_SVC 0
#define SCHEDULER_CREATE_SVC 1
#define SCHEDULER_YIELD_SVC 2
#define SCHEDULER_TERMINATE_SVC 3
#define SCHEDULER_SUSPEND_SVC 4
#define SCHEDULER_RESUME_SVC 5
#define SCHEDULER_WAIT_SVC 6
#define SCHEDULER_WAKE_SVC 7
#define SCHEDULER_PRIORITY_SVC 8

#define SCHEDULER_PRIOR_BITS 0x00000002UL

#define SCHEDULER_MAX_IRQ_PRIORITY INTERRUPT_ABOVE_NORMAL
#define SCHEDULER_MIN_IRQ_PRIORITY INTERRUPT_BELOW_NORMAL

#define SCHEDULER_PENDSV_PRIORITY (SCHEDULER_MIN_IRQ_PRIORITY)
#define SCHEDULER_SVC_PRIORITY (SCHEDULER_MIN_IRQ_PRIORITY - 1)

#define SCHEDULER_NUM_TASK_PRIORITIES 32UL
#define SCHEDULER_MAX_TASK_PRIORITY 0UL
#define SCHEDULER_MIN_TASK_PRIORITY (SCHEDULER_NUM_TASK_PRIORITIES - 1)

#define SCHEDULER_MARKER 0x13700731UL
#define SCHEDULER_TASK_MARKER 0x137aa731UL
#define SCHEDULER_FUTEX_MARKER 0x137bb731UL
#define SCHEDULER_STACK_MARKER 0x137cc731UL

#define SCHEDULER_WAIT_FOREVER 0xffffffffUL
#define SCHEDULER_IGNORE_VIABLE 0x00000001UL
#define SCHEDULER_TASK_STACK_CHECK 0x00000002UL

#define SCHEDULER_FUTEX_CONTENTION_TRACKING 0x00000001UL
#define SCHEDULER_FUTEX_PI 0x00000002UL
#define SCHEDULER_FUTEX_OWNER_TRACKING 0x00000004UL

#ifndef SCHEDULER_MAX_DEFERED_WAKE
#define SCHEDULER_MAX_DEFERED_WAKE 8
#endif

#ifndef SCHEDULER_TIME_SLICE
#define SCHEDULER_TIME_SLICE INT32_MAX
#endif

#ifndef SCHEDULER_MAIN_STACK_SIZE
#define SCHEDULER_MAIN_STACK_SIZE 4096UL
#endif

#ifndef SCHEDULER_TICK_FREQ
#define SCHEDULER_TICK_FREQ 1000UL
#endif

struct exception_frame
{
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t psr;
};

struct scheduler_frame
{
	uint32_t exec_return;
	uint32_t control;

	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;

	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;


	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t psr;
};

struct sched_list
{
	struct sched_list *next;
	struct sched_list *prev;
};

struct sched_queue
{
	unsigned int size;
	struct sched_list tasks;
};

enum task_state
{
	TASK_OTHER = -1,
	TASK_TERMINATED = 0,
	TASK_RUNNING = 1,
	TASK_READY = 2,
	TASK_BLOCKED = 3,
	TASK_SLEEPING = 4,
	TASK_SUSPENDED = 5,
	TASK_RESERVED = 0x7fffffff,
};

typedef union
{
	uint64_t jiffies;
	uint32_t ticks[2];
} counter64_t;

struct task;
typedef void (*task_entry_point_t)(void *context);
typedef void (*task_exit_handler_t)(struct task *task);
typedef bool (*for_each_sched_node_t)(struct sched_list *node, void *context);

struct task_descriptor
{
	char name[TASK_NAME_LEN];
	task_entry_point_t entry_point;
	task_exit_handler_t exit_handler;
	void *context;
	unsigned long flags;
	unsigned long priority;
};

struct task
{
	/* This must be the first field, PendSV depends on it */
	struct scheduler_frame *psp;
	void *tls;
	unsigned long *stack_marker;

	enum task_state state;

	unsigned long base_priority;
	unsigned long current_priority;
	atomic_int slice_expires;

	counter64_t timer_expires;
	struct sched_list timer_node;

	struct sched_queue *current_queue;
	struct sched_list queue_node;

	struct sched_list scheduler_node;

	struct sched_list owned_futexes;

	void *context;
	task_exit_handler_t exit_handler;
	char name[TASK_NAME_LEN];
	size_t stack_size;
	atomic_ulong flags;

	unsigned long marker;
};

struct futex
{
	long *value;
	struct sched_queue waiters;
	struct sched_list owned;
	unsigned long flags;
	unsigned long marker;
};

struct scheduler
{
	/* This must be the first field, PendSV depends on it */
	struct task *current;

	/* Must be the second field the scheduler start service depends on it */
	struct scheduler_frame *initial_frame;

	size_t tls_size;
	unsigned long slice_duration;

	struct sched_queue ready_queue;
	struct sched_queue suspended_queue;
	struct sched_queue sleep_queue;

	struct sched_list tasks;
	struct sched_list timers;
	volatile counter64_t timer_expires;

	atomic_int locked;
	volatile counter64_t time;

	unsigned long deferred_wake[SCHEDULER_MAX_DEFERED_WAKE];

	unsigned long marker;
};

static inline __always_inline unsigned long scheduler_enter_critical(void)
{
	return disable_interrupts();
}

static inline __always_inline void scheduler_exit_critical(unsigned long status)
{
	enable_interrupts(status);
}

int scheduler_init(struct scheduler *new_scheduler, size_t tls_size);
int scheduler_run(void);
bool scheduler_is_running(void);
void scheduler_for_each(struct sched_list *list, for_each_sched_node_t func, void *context);

unsigned long long scheduler_jiffies(void);
unsigned long scheduler_ticks(void);

struct task *scheduler_create(void *stack, size_t stack_size, const struct task_descriptor *descriptor);
struct task *scheduler_task(void);

int scheduler_lock(void);
int scheduler_unlock(void);
int scheduler_lock_restore(int lock);
bool scheduler_is_locked(void);

void scheduler_yield(void);
int scheduler_sleep(unsigned long ticks);

int scheduler_suspend(struct task *task);
int scheduler_resume(struct task *task);
int scheduler_terminate(struct task *task);

void scheduler_futex_init(struct futex *futex, long *value, unsigned long flags);
int scheduler_futex_wait(struct futex *futex, long value, unsigned long ticks);
int scheduler_futex_wake(struct futex *futex, bool all);

int scheduler_set_priority(struct task *task, unsigned long priority);
unsigned long scheduler_get_priority(struct task *task);

void scheduler_set_flags(struct task *task, unsigned long mask);
void scheduler_clear_flags(struct task *task, unsigned long mask);
unsigned long scheduler_get_flags(struct task *task);

#endif