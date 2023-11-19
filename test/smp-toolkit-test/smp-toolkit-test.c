#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sys/syslog.h>
#include <sys/systick.h>
#include <sys/swi.h>
#include <sys/irq.h>

#include <init/init-sections.h>

#include <rtos/rtos.h>

#include <unistd.h>

#define NUM_HOGS 8

struct arguments
{
	int argc;
	char **argv;
	int ret;
};

struct hog
{
	struct task *id;
	unsigned int loops;
	unsigned int cores[2];
};

struct hog hogs[NUM_HOGS] = { 0 };
unsigned int swi_handler_counter = 0;
unsigned int swi_kick_counter = 0;
unsigned int swi_cores[2] = { 0, 0};
struct task *swi_counter_id = 0;
long events = 0;
struct futex futex;

static void swi_handler(unsigned int swi, void *context)
{
	events = 1;
	int status = scheduler_futex_wake(&futex, true);
	if (status < 0)
		syslog_fatal("failed to wake futex: %d\n", status);
	++swi_handler_counter;
}

static void swi_counter_task(void *context)
{
	while (true) {
		int status = scheduler_futex_wait(&futex, 0, SCHEDULER_WAIT_FOREVER);
		if (status < 0)
			syslog_fatal("failed to wait for futex: %d\n", status);
		++swi_cores[SystemCurrentCore()];
		events = 0;
	}
}

static void hog_task(void *context)
{
	struct hog *hog = context;

	while (true) {
		unsigned int core = SystemCurrentCore();
		++hog->loops;
		++hog->cores[core];
		if ((random() & 0x8) == 0) {
			swi_trigger(5);
			++swi_kick_counter;
			scheduler_yield();
		}
	}
}

int main(int argc, char **argv)
{
	irq_set_core(SWI_5_IRQn, 1);
	swi_register(5, INTERRUPT_NORMAL, swi_handler, 0);
	swi_enable(5);

	scheduler_futex_init(&futex, &events, 0);

	struct task_descriptor swi_counter_task_desc = { .entry_point = swi_counter_task, .context = &futex, .priority = SCHEDULER_MIN_TASK_PRIORITY / 4 };
	strncpy(swi_counter_task_desc.name, "swi-counter", TASK_NAME_LEN);
	swi_counter_id = scheduler_create(sbrk(1024), 1024, &swi_counter_task_desc);
	if (!swi_counter_id)
		syslog_fatal("failed to start swi_counter_task\n");

	syslog_info("launching %u hogs\n", NUM_HOGS);
	for (int i = 0; i < NUM_HOGS; ++i) {
		struct task_descriptor hog_task_desc = { .entry_point = hog_task, .context = &hogs[i], .priority = SCHEDULER_MIN_TASK_PRIORITY / 2 };
		strncpy(hog_task_desc.name, "hog", TASK_NAME_LEN);
		hogs[i].id = scheduler_create(sbrk(1024), 1024, &hog_task_desc);
		if (!hogs[i].id)
			syslog_fatal("failed to start hog %d\n", i);
	}

	/* Dump hog info */
	while (true) {
		printf("---\n");
		printf("\tswi = [%u, %u, %u, %u]\n", swi_kick_counter, swi_handler_counter, swi_cores[0], swi_cores[1]);
		for (int i = 0; i < NUM_HOGS; ++i)
			printf("\thog[%d] = [%u, %u]\n", i, hogs[i].cores[0], hogs[i].cores[1]);
		scheduler_sleep(1000);
	}

	return EXIT_SUCCESS;
}

static void main_task(void *context)
{
	assert(context != 0);

	struct arguments *args = context;

	args->ret = main(args->argc, args->argv);
}

int _main(int argc, char **argv);
int _main(int argc, char **argv)
{
	struct scheduler scheduler;

	/* Initialize the scheduler */
	int status = scheduler_init(&scheduler, (size_t)&__tls_size);
	if (status < 0)
		return status;

	/* Execute the ini array */
	run_init_array(__init_array_start, __init_array_end);

	/* Setup the main task */
	struct arguments args = { .argc = argc, .argv = argv, .ret = 0 };
	struct task_descriptor main_task_descriptor;
	strcpy(main_task_descriptor.name, "main-task");
	main_task_descriptor.entry_point = main_task;
	main_task_descriptor.exit_handler = 0;
	main_task_descriptor.context = &args;
	main_task_descriptor.flags = 0;
	main_task_descriptor.priority = SCHEDULER_MAX_TASK_PRIORITY;
	struct task *main_task = scheduler_create(sbrk(SCHEDULER_MAIN_STACK_SIZE), SCHEDULER_MAIN_STACK_SIZE, &main_task_descriptor);
	if (!main_task) {
		errno = EINVAL;
		return -EINVAL;
	}

	/* Run the scheduler */
	status = scheduler_run();
	if (status == 0)
		status = args.ret;

	/* Execute the fini array */
	run_init_array(__fini_array_start, __fini_array_end);

	return status;
}
