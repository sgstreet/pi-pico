#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <container-of.h>
#include <malloc.h>
#include <linked-list.h>
#include <unistd.h>

#include <sys/fault.h>
#include <sys/syslog.h>
#include <hal/hal.h>

#include <rtos/rtos.h>

#define STACK_SIZE 1024
#define NUM_STACKS 20

struct core_data {
	uint32_t marker;
	uint32_t systicks;
	uint32_t systick_handlers[8];
	uint32_t scheduler_initial_frame;
	uint32_t current_task;
	uint32_t slice_expires;
	void *tls;
};

struct trace_entry
{
	unsigned int tag;
	void *mem_addr;
	const char *invoker;
};

extern __weak unsigned int scheduler_spin_lock_irqsave(void);
extern __weak void scheduler_spin_unlock_irqrestore(unsigned int state);
__noreturn void __assert_fail(const char *expr);

extern void *_cls;
struct core_data *core_data = 0;
core_local uint32_t marker = 0xdeadbeef;

extern struct scheduler* scheduler;
static __unused struct linked_list stack_pool = LIST_INIT(stack_pool);
static __unused osOnceFlag_t stack_pool_once = osOnceFlagsInit;

struct trace_entry trace[128];
atomic_uint trace_index = 0;

__noreturn void __assert_fail(const char *expr)
{
	backtrace_t backtrace[25];
	int count = backtrace_unwind(backtrace, 25);

	__BKPT(100);

	fprintf(stderr, "assert failted: %s\n", expr);
	for (int i = 0; i < count; ++i)
		fprintf(stderr, "%p - %s + %u\n", backtrace[i].address, backtrace[i].name, backtrace[i].address - backtrace[i].function);

//	size_t idx = (trace_index - 1) & 127;
//	for (int i = 0; i < 10; i++) {
//		fprintf(stderr, "tag: %u invoker: %s addr: %p\n", trace[idx].tag, trace[idx].invoker, trace[idx].mem_addr);
//		idx = (idx - 1) & 127;
//	}

	abort();
}

__weak unsigned int scheduler_spin_lock_irqsave(void)
{
	return disable_interrupts();
}

__weak void scheduler_spin_unlock_irqrestore(unsigned int state)
{
	enable_interrupts(state);
}

void *_rtos2_alloc(size_t size);
void *_rtos2_alloc(size_t size)
{
	return calloc(1, size);
}

void _rtos2_release(void *ptr);
void _rtos2_release(void *ptr)
{
	free(ptr);
}

static __unused void stack_pool_init(void)
{
	static char stack_space[NUM_STACKS * STACK_SIZE];
	memset(stack_space, 0xaa, STACK_SIZE);
	for (size_t i = 0; i < NUM_STACKS; ++i)
		list_push(&stack_pool, (struct linked_list *)(stack_space + (i * STACK_SIZE)));
}

int outstanding = 0;
struct rtos_thread *_rtos2_alloc_thread(size_t stack_size);
struct rtos_thread *_rtos2_alloc_thread(size_t stack_size)
{
//	backtrace_t backtrace[2];
//	backtrace_unwind(backtrace, 2);
//	size_t idx = atomic_fetch_add(&trace_index, 1) & 127;
//
	/* Calculate the required size needed to provide the request stack size and make it a multiple of 8 bytes */
	__unused size_t thread_size = sizeof(struct rtos_thread) + stack_size;
	return calloc(1, thread_size);
//
//	/* Make sure the pool is initialized */
//	osCallOnce(&stack_pool_once, stack_pool_init);
//
//	struct rtos_thread *thread = 0;
//	while (!thread) {
//		uint32_t state = osKernelEnterCritical();
//		thread = (struct rtos_thread *)list_pop(&stack_pool);
//		outstanding += thread != 0 ? 1 : 0;
//		osKernelExitCritical(state);
//		osThreadYield();
//	}
//
//	assert(thread != 0);
//
//	memset(thread, 0xaa, STACK_SIZE);
//
//	trace[idx].tag = 1;
//	trace[idx].mem_addr = thread;
//	trace[idx].invoker = backtrace[1].name;
//
//	return thread;
}

void _rtos2_release_thread(struct rtos_thread *thread);
void _rtos2_release_thread(struct rtos_thread *thread)
{
	free(thread);
//	assert(thread != 0);
//
//	backtrace_t backtrace[2];
//	backtrace_unwind(backtrace, 2);
//	size_t idx = atomic_fetch_add(&trace_index, 1) & 127;
//
//	memset(thread, 0xbb, STACK_SIZE);
//
//	struct linked_list *node = (struct linked_list *)thread;
//
//	uint32_t state = osKernelEnterCritical();
//	list_push(&stack_pool, node);
//	--outstanding;
//	osKernelExitCritical(state);
//
//	trace[idx].tag = 2;
//	trace[idx].mem_addr = thread;
//	trace[idx].invoker = backtrace[1].name;
}

__noreturn void abort(void);
__noreturn void abort(void)
{
	__BKPT(100);
	while (true);
}

void _rtos2_thread_stack_overflow(struct rtos_thread *thread);
void _rtos2_thread_stack_overflow(struct rtos_thread *thread)
{
	abort();
}

void save_fault(const struct cortexm_fault *fault, const struct backtrace *backtrace, int count);
void save_fault(const struct cortexm_fault *fault, const struct backtrace *backtrace, int count)
{
	fprintf(stderr, "faulted\n");
	for (int i = 0; i < count; ++i)
		fprintf(stderr, "%p - %s + %u\n", backtrace[i].address, backtrace[i].name, backtrace[i].address - backtrace[i].function);
}

static __unused bool dump_threads(struct sched_list *node, void *context)
{
	struct task* task = container_of(node, struct task, scheduler_node);
	printf("task %s@%p state: %u priority: %lu task marker: 0x%08lx rtos: %p rtos marker: 0x%08lx\n", task->name, task, task->state, task->current_priority, task->marker, task->context, *(uint32_t *)(task->context));
	return true;
}

static __unused void delay_forever(void *arg)
{
	osDelay(osWaitForever);
	abort();
}

static __unused void self_terminate(void *arg)
{
	osThreadTerminate(osThreadGetId());
	abort();
}

static __unused void thread_exit(void *arg)
{
}

static __unused void thread_busy(void *arg)
{
	while (true) {
		osStatus_t os_status = osIsResourceValid(osThreadGetId(), RTOS_THREAD_MARKER);
		if (os_status != osOK)
			abort();
	}
}

static __unused void thread_yielding(void *arg)
{
	while (true) {
		osStatus_t os_status = osThreadYield();
		if (os_status != osOK)
			abort();
	}
}

static __unused void thread_suspended(void *arg)
{
	osStatus_t os_status = osIsResourceValid(osThreadGetId(), RTOS_THREAD_MARKER);
	if (os_status != osOK)
		abort();
	osThreadSuspend(osThreadGetId());
}

static __unused void test6(void)
{
	osThreadAttr_t attr = { NULL, osThreadDetached, NULL, 0U, NULL, 0U, osPriorityNormal, 0U, 0U };
	osThreadId_t tid;
	osPriority_t prio;
	osStatus_t status;
	char name[32];
	static int count = 0;
//	void (*styles[])(void *context) = { delay_forever, self_terminate, thread_exit, thread_busy, thread_yielding, thread_suspended};

	/* Call osThreadNew and use attributes to create thread with user specified priority */
	for (prio = osPriorityLow; prio < osPriorityISR; prio++) {

		/* Set priority and name */
		sprintf(name, "test-6-%u", prio);
		attr.priority = prio;
		attr.name = name;

		tid = osThreadNew(thread_suspended, NULL, &attr);

//		scheduler_for_each(&scheduler->tasks, dump_threads, 0);
		++count;
//		struct task *task = ((struct rtos_thread *)tid)->stack;
//		printf("%d - task %s@%p state: %u priority: %lu task marker: 0x%08lx rtos: %p rtos marker: 0x%08lx\n", count, task->name, task, task->state, task->current_priority, task->marker, task->context, *(uint32_t *)(task->context));

		/* Verify that thread was successfully created */
		assert(tid != 0);

		/* Terminate thread */
		status = osThreadTerminate(tid);
		if (status != osOK)
			scheduler_for_each(&scheduler->tasks, dump_threads, 0);
		assert(status == osOK);

//		status = osThreadJoin(tid);
//		assert(status == osOK);
	}
}

static __unused void stack_size1(void)
{
	osThreadAttr_t attr = { "stack-size1", osThreadDetached, NULL, 0U, NULL, 128U, osPriorityLow, 0U, 0U };
	osThreadId_t id;
	static int crt_cnt = 0;

	++crt_cnt;

	/* Call osThreadGetStackSize to retrieve the stack size of a running thread */
	id = osThreadGetId();
	uint32_t stack_size = osThreadGetStackSize(id);
	assert(stack_size == 4096);

	/* Create a thread with a stack size of 128 bytes */
	id = osThreadNew(thread_exit, NULL, &attr);
	assert(id != NULL);

	/* Call osThreadGetStackSize to retrieve the stack size of a 'Ready' thread */
	stack_size = osThreadGetStackSize(id);
	assert(stack_size == 128U);

	/* Terminate thread */
	osThreadTerminate(id);

	/* Call osThreadGetStackSize with null object */
	stack_size = osThreadGetStackSize(NULL);
	assert(osThreadGetStackSize(NULL) == 0U);
}

static __unused void joinable_termination_test(void)
{
	struct rtos_thread *id;
	osStatus_t os_status;
	char name[32];
	osThreadAttr_t attr = { .name = name, .attr_bits = osThreadJoinable, .stack_size = 0, .priority = osPriorityLow };
	void (*styles[])(void *context) = { thread_exit, delay_forever, self_terminate, thread_busy, thread_yielding, thread_suspended};

	/* First joinable */
	for (int style = 0; style < array_sizeof(styles); ++style) {

		sprintf(name, "j-termination-test-%d", style);

		id = osThreadNew(styles[style], 0, &attr);
		assert(id != 0);

		os_status  = osThreadTerminate(id);
		assert(os_status == osOK);

		os_status = osThreadJoin(id);
		assert(os_status == osOK);
	}

}

static __unused void detached_termination_test(void)
{
	struct rtos_thread *id;
	osStatus_t os_status;
	char name[32];
	osThreadAttr_t attr = { .name = name, .attr_bits = osThreadDetached, .stack_size = 0, .priority = osPriorityLow };
	void (*styles[])(void *context) = { delay_forever, thread_busy, thread_yielding, thread_suspended};

	/* Now detached, skip exiting thread as this is racy */
	for (int style = 0; style < array_sizeof(styles); ++style) {

		sprintf(name, "d-termination-test-%d", style);

		id = osThreadNew(styles[style], 0, &attr);
		assert(id != 0);

		os_status  = osThreadTerminate(id);
		assert(os_status == osOK || os_status == osErrorParameter);
	}
}

int main(int argc, char **argv)
{
	core_data = _cls;
	__unused struct rtos_thread *main_id = osThreadGetId();
	int cnt = 0;
	while (true) {
		printf("marker: 0x%08lx running %d\n", cls_datum(marker), ++cnt);
		joinable_termination_test();
		detached_termination_test();
		test6();
	}
}
