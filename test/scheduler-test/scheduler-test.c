#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <container-of.h>
#include <malloc.h>
#include <unistd.h>
#include <linked-list.h>

#include <init/init-sections.h>

#include <sys/fault.h>
#include <sys/syslog.h>
#include <sys/delay.h>

#include <hal/hal.h>
#include <rtos/rtos.h>

#define TASK_HIGH_PRIORITY 32UL
#define TASK_LOW_PRIORITY 48UL

struct arguments
{
	int argc;
	char **argv;
	int ret;
};

struct core_data {
	uint32_t marker;
	uint32_t systicks;
	uint32_t systick_handlers[8];
	uint32_t scheduler_initial_frame;
	uint32_t current_task;
	uint32_t slice_expires;
	void *tls;
};

extern void _init();
extern void _fini();
extern unsigned int scheduler_spin_lock_irqsave(void);
extern void scheduler_spin_unlock_irqrestore(unsigned int state);

int main(int argc, char **argv);

core_local uint32_t marker = 0xdeadbeef;
extern void *_cls;
struct core_data *core_data = 0;

extern struct scheduler *scheduler;

static void *stack_space = 0;
struct linked_list task_stacks = LIST_INIT(task_stacks);

__noreturn void __assert_fail(const char *expr);
__noreturn void __assert_fail(const char *expr)
{
	backtrace_t backtrace[25];
	backtrace_unwind(backtrace, 25);
	abort();
}

__noreturn void abort(void);
__noreturn void abort(void)
{
	__BKPT(100);
	while (true);
}

__weak unsigned int scheduler_spin_lock_irqsave(void)
{
	return disable_interrupts();
}

__weak void scheduler_spin_unlock_irqrestore(unsigned int state)
{
	enable_interrupts(state);
}

static __unused bool dump_threads(struct sched_list *node, void *context)
{
	struct task* task = container_of(node, struct task, scheduler_node);
	printf("task %s@%p state: %u priority: %lu\n", task->name, task, task->state, task->current_priority);
	return true;
}

void main_task(void *context);
void main_task(void *context)
{
	assert(context != 0);

	struct arguments *args = context;

	_init();

	/* Execute the ini array */
	run_init_array(__init_array_start, __init_array_end);

	/* Initialize the iso thread, this will wrap the main task */
	args->ret = main(args->argc, args->argv);

	/* Execute the fini array */
	run_init_array(__fini_array_start, __fini_array_end);

	_fini();
}

int _main(int argc, char **argv);
int _main(int argc, char **argv)
{
	struct scheduler scheduler;

	/* Initialize the scheduler */
	int status = scheduler_init(&scheduler, (size_t)&__tls_size);
	if (status < 0)
		return status;

	/* Setup the main task */
	struct arguments args = { .argc = argc, .argv = argv, .ret = 0 };
	struct task_descriptor main_task_descriptor;
	strcpy(main_task_descriptor.name, "main-task");
	main_task_descriptor.entry_point = main_task;
	main_task_descriptor.exit_handler = 0;
	main_task_descriptor.context = &args;
	main_task_descriptor.flags = SCHEDULER_TASK_STACK_CHECK;
	main_task_descriptor.priority = TASK_HIGH_PRIORITY + 1;
	struct task *main_task = scheduler_create(alloca(SCHEDULER_MAIN_STACK_SIZE), SCHEDULER_MAIN_STACK_SIZE, &main_task_descriptor);
	if (!main_task) {
		errno = EINVAL;
		return -EINVAL;
	}

	/* Run the scheduler, scheduler will exit with the last viable task */
	status = scheduler_run();
	if (status == 0)
		status = args.ret;

	/* return the status from the main task */
	return status;
}

static struct task *create_task(const char *name, task_entry_point_t entry_point, task_exit_handler_t exit_handler, unsigned long priority, void *context)
{
	/* Set up the descriptor */
	struct task_descriptor desc = { .entry_point = entry_point, .exit_handler = exit_handler, .priority = priority, .context = context, .flags = SCHEDULER_TASK_STACK_CHECK };
	strncpy(desc.name, name, sizeof(desc.name));

	unsigned int state = scheduler_spin_lock_irqsave();
	void *stack = list_pop(&task_stacks);
	if (!stack)
		abort();
	scheduler_spin_unlock_irqrestore(state);

	return scheduler_create(stack, 1024, &desc);
}

static volatile int counter;
static __unused void task_exit_handler(struct task *task)
{
	/* We should already be holding the scheduler spin lock */
	assert(task->state == TASK_TERMINATED);
//	if (task == (void *)0x20004028)
//		__BKPT(100);

//	memset(task, -1, 1024);
	task->marker = 0;
	list_push(&task_stacks, (struct linked_list *)task);
	--counter;
}

static volatile bool task_busy_run = true;
static __unused void task_busy(void *context)
{
	__unused struct task *task = scheduler_task();
	while (task_busy_run);
	scheduler_terminate(0);
}

static __unused void task_suspend(void *context)
{
	scheduler_suspend(0);
}

static __unused void task_yielding(void *context)
{
	while (true)
		scheduler_yield();
}

static __unused void task_self_terminated(void *context)
{
	scheduler_terminate(0);
}

static struct delay_stats {
	unsigned long count;
	unsigned long core_count[2];
	long min_delta;
	long max_delta;
} delayer_stats[10] = { 0 };
static unsigned int seed = 137;

static __unused void task_delayer(void *context)
{
	struct delay_stats *stats = context;

	stats->min_delta = UINT32_MAX;

	while (true) {
		int delay = (rand_r(&seed) % 10);

		unsigned long ticks = scheduler_get_ticks();
		scheduler_sleep(delay);
		long actual_delta = delay - (scheduler_get_ticks() - ticks);

		if (actual_delta > stats->max_delta)
			stats->max_delta = actual_delta;

		if (actual_delta < stats->min_delta)
			stats->min_delta = actual_delta;

		++stats->count;
		++stats->core_count[SystemCurrentCore()];
	}
}

static int check_state(struct task* task, enum task_state desired)
{
	enum task_state task_state = task->state;
	int timeout = 10000;
	while (--timeout > 0) {
		task_state = task->state;
		if (task_state == desired)
			return timeout;
//		delay_cycles(1);//(SystemCoreClock/1000000);
//		scheduler_yield();
	}
	abort();
}

static void check_counter(int value)
{
	int timeout = 1000;
	while (--timeout) {
		if (counter == value)
			return;
//		scheduler_yield();
	}
	abort();
}

static __unused void delay_test(void)
{
	struct task *tasks[array_sizeof(delayer_stats) - 1] = { 0 };

	delayer_stats[array_sizeof(delayer_stats) - 1].min_delta = UINT32_MAX;
	for (int i = 0; i < 5; ++i) {

		unsigned long ticks = scheduler_get_ticks();
		scheduler_sleep(100);
		unsigned long delay = scheduler_get_ticks() - ticks;

		++delayer_stats[array_sizeof(delayer_stats) - 1].count;

		if (delay > delayer_stats[array_sizeof(delayer_stats) - 1].max_delta)
			delayer_stats[array_sizeof(delayer_stats) - 1].max_delta = delay;

		if (delay < delayer_stats[array_sizeof(delayer_stats) - 1].min_delta)
			delayer_stats[array_sizeof(delayer_stats) - 1].min_delta = delay;
	}

	for (int i = 0; i < array_sizeof(delayer_stats) - 1; ++i) {
		tasks[i] = create_task("delayer", task_delayer, task_exit_handler, 0, &delayer_stats[i]);
		if (!tasks[i])
			abort();
	}

	scheduler_sleep(500);

	counter = array_sizeof(delayer_stats);
	for (int i = 0; i < array_sizeof(delayer_stats) - 1; ++i) {
		int status = scheduler_terminate(tasks[i]);
		if (status < 0)
			abort();
	}
	check_counter(1);
}

static __unused void terminate_test_with_priority(void)
{
	char name[32];

	for (unsigned long priority = SCHEDULER_MIN_TASK_PRIORITY; priority != UINT32_MAX; --priority) {
		sprintf(name, "ttwp-%lu", priority);
		counter = 1;
		__unused struct task *task = create_task(name, task_self_terminated, task_exit_handler, priority, 0);
		check_counter(0);
	}
}

static __unused void suspend_test_with_priority(void)
{
	char name[32];

	for (unsigned long priority = SCHEDULER_MIN_TASK_PRIORITY; priority != UINT32_MAX; --priority) {
		sprintf(name, "ttwp-%lu", priority);
		counter = 1;
		__unused struct task *task = create_task(name, task_suspend, task_exit_handler, priority, 0);
		check_state(task, TASK_SUSPENDED);
		scheduler_terminate(task);
		check_counter(0);
	}
}

static __unused void termination_test(void)
{
	int status;

	counter = 4;

	/* Create a high priority busy task to keep other for busy */
	struct task* hp_task = create_task("high-priority", task_yielding, task_exit_handler, TASK_HIGH_PRIORITY, 0);
	status = check_state(hp_task, TASK_RUNNING);
//	printf("timeout = %d\n", status);

	/* Now create a low priority task */
	struct task* lp_task = create_task("low-priority", task_busy, task_exit_handler, TASK_LOW_PRIORITY, 0);
	check_state(lp_task, TASK_READY);

	/* Create a suspended task */
	struct task* suspended_task = create_task("suspended-task", task_suspend, task_exit_handler, TASK_HIGH_PRIORITY, 0);
	check_state(suspended_task, TASK_SUSPENDED);

	/* Create a terminated task */
	struct task* terminated_task = create_task("terminated-task", task_self_terminated, task_exit_handler, TASK_HIGH_PRIORITY, 0);

	check_counter(3);
	status = scheduler_terminate(terminated_task);
	assert(status == -ESRCH);

	status = scheduler_terminate(lp_task);
	assert(status == 0);
	check_counter(2);

	status = scheduler_terminate(suspended_task);
	assert(status == 0);
	check_counter(1);

	status = scheduler_terminate(hp_task);
	assert(status == 0);
	check_counter(0);
}

static __unused void suspend_test(void)
{
	task_busy_run = true;
	int status;

	/* Create a high priority busy task to keep other for busy */
	struct task* hp_task = create_task("high-priority", task_busy, task_exit_handler, TASK_HIGH_PRIORITY, 0);
	status = check_state(hp_task, TASK_RUNNING);
//	printf("timeout = %d\n", status);

	/* Now create a low priority task */
	struct task* lp_task = create_task("low-priority", task_busy, task_exit_handler, TASK_LOW_PRIORITY, 0);
	check_state(lp_task, TASK_READY);

	/* Suspend low priority task */
	status = scheduler_suspend(lp_task);
	assert(status == 0);
	check_state(lp_task, TASK_SUSPENDED);

	/* Resume it */
	status = scheduler_resume(lp_task);
	assert(status == 0);
	check_state(lp_task, TASK_READY);

	/* Suspend high priority task, to let the low priority task run */
	status = scheduler_suspend(hp_task);
	assert(status == 0);
	check_state(hp_task, TASK_SUSPENDED);
	check_state(lp_task, TASK_RUNNING);

	/* Resume the high priority task */
	status = scheduler_resume(hp_task);
	assert(status == 0);
	check_state(hp_task, TASK_RUNNING);
	check_state(lp_task, TASK_READY);

	counter = 2;
	task_busy_run = false;
	while (counter > 0);
}

int main(int argc, char **argv)
{
	core_data = _cls;

	/* Initialize stack space */
	stack_space = sbrk(10 * 1024);
	memset(stack_space, -1, 1024);
	for (size_t i = 0; i < 10; ++i)
		list_push(&task_stacks, stack_space + (i * 1024));

	scheduler->slice_duration = UINT32_MAX;
	int cnt = 0;
	while (true) {
		printf("run: %d\n",++cnt);
		suspend_test();
		termination_test();
		delay_test();
		suspend_test_with_priority();
		terminate_test_with_priority();
	}
}
