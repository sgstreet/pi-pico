/*
 * debug-cmds.c
 *
 *  Created on: Apr 26, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <cmsis/cmsis.h>
#include <sys/tls.h>
#include <sys/backtrace.h>

#include <rtos/rtos-toolkit/scheduler.h>

#include <svc/shell.h>

static int cpu_active(int argc, char **argv)
{
	printf("%lu\n", SIO->CPUID);
	return EXIT_SUCCESS;
}

static __shell_command const struct shell_command cpu_active_cmd =
{
	.name = "cpu-active",
	.usage = "",
	.func = cpu_active,
};

static int mem_read(int argc, char **argv)
{
	size_t width = 4;
	char option_value;
	char *end;
	void *addr;
	size_t count = 1;
	struct option long_options[] =
	{
		{
			.name = "width",
			.has_arg = required_argument,
			.flag = 0,
			.val = 'w',
		},
		{
			.name = 0,
			.has_arg = 0,
			.flag = 0,
			.val = 0,
		},
	};

	/* Get the optional width */
	optind = 0;
	while ((option_value = getopt_long(argc, argv, "w:", long_options, 0)) != -1)
		switch (option_value) {
			case 'w':
				width = strtoul(optarg, &end, 0);
				if (errno != 0 || end == optarg) {
					fprintf(stderr, "bad width: '%s'\n", optarg);
					return EXIT_FAILURE;
				}
				break;
			default:
				fprintf(stderr, "bad option: %c\n", option_value);
				return EXIT_FAILURE;
		}

	/* Get the address to use */
	if (optind == argc) {
		fprintf(stderr, "missing required address\n");
		return EXIT_FAILURE;
	}
	errno = 0;
	addr = (void *)strtoul(argv[optind], &end, 0);
	if (errno != 0 || end == argv[optind]) {
		fprintf(stderr, "bad address: '%s'\n", argv[optind]);
		return EXIT_FAILURE;
	}
	++optind;

	/* Get the optional count */
	if (optind < argc) {
		errno = 0;
		count = strtoul(argv[optind], &end, 0);
		if (errno != 0 || end == argv[optind]) {
			fprintf(stderr, "bad count: '%s'\n", argv[optind]);
			return EXIT_FAILURE;
		}
		++optind;
	}

	/* Read and print the data */
	for (int i = 0; i < count; ++i) {
		switch(width) {
			case 1:
				printf("%p - 0x%02hhx\n", addr, *((uint8_t *)addr));
				addr += 1;
				break;
			case 2:
				printf("%p - 0x%04hx\n", addr, *((uint16_t *)addr));
				addr += 2;
				break;
			case 4:
				printf("%p - 0x%08lx\n", addr, *((uint32_t *)addr));
				addr += 4;
				break;
			case 8:
				printf("%p - 0x%016llx\n", addr, *((uint64_t *)addr));
				addr += 8;
				break;
			default:
				fprintf(stderr, "bad width: '%s'\n", optarg);
				return EXIT_FAILURE;
		}
	}

	/* All good */
	return 0;
}

static __shell_command const struct shell_command mem_read_cmd =
{
	.name = "mem-read",
	.usage = "[-w | --width 1 2 4 8] <addr> [count]",
	.func = mem_read,
};

static int mem_write(int argc, char **argv)
{
	size_t width = 4;
	char option_value;
	char *end;
	void *addr;
	uint64_t value;
	struct option long_options[] =
	{
		{
			.name = "width",
			.has_arg = required_argument,
			.flag = 0,
			.val = 'w',
		},
		{
			.name = 0,
			.has_arg = 0,
			.flag = 0,
			.val = 0,
		},
	};

	/* Get the optional width */
	optind = 0;
	while ((option_value = getopt_long(argc, argv, "w:", long_options, 0)) != -1)
		switch (option_value) {
			case 'w':
				width = strtoul(optarg, &end, 0);
				if (errno != 0 || end == optarg) {
					fprintf(stderr, "bad width: '%s'\n", optarg);
					return EXIT_FAILURE;
				}
				break;
			default:
				fprintf(stderr, "bad option: %c\n", option_value);
				return EXIT_FAILURE;
		}

	/* Get the address to use */
	if (optind == argc) {
		fprintf(stderr, "missing required address\n");
		return EXIT_FAILURE;
	}
	errno = 0;
	addr = (void *)strtoul(argv[optind], &end, 0);
	if (errno != 0 || end == argv[optind]) {
		fprintf(stderr, "bad address: '%s'\n", argv[optind]);
		return EXIT_FAILURE;
	}
	++optind;

	/* Get the optional count */
	if (optind == argc) {
		fprintf(stderr, "missing required value\n");
		return EXIT_FAILURE;
	}
	errno = 0;
	value = strtoll(argv[optind], &end, 0);
	if (errno != 0 || end == argv[optind]) {
		fprintf(stderr, "bad value: '%s'\n", argv[optind - 1]);
		return EXIT_FAILURE;
	}
	++optind;

	/* Write the data */
	switch(width) {
		case 1:
			*((uint8_t*)addr) = value & 0xff;
			break;
		case 2:
			*((uint16_t*)addr) = value & 0xffff;
			break;
		case 4:
			*((uint32_t*)addr) = value & 0xffffffff;
			break;
		case 8:
			*((uint16_t*)addr) = value;
			break;
		default:
			fprintf(stderr, "bad width: '%s'\n", optarg);
			return EXIT_FAILURE;
	}

	/* All good */
	return 0;
}

static __shell_command const struct shell_command mem_write_cmd =
{
	.name = "mem-write",
	.usage = "[-w | --width 1 2 4 8] <addr> <value>",
	.func = mem_write,
};

static int cls_dump(int argc, char **argv)
{
	extern void *__core_data_size;
//	extern void *__aeabi_read_core_cls(unsigned long core);

	for (size_t i = 0; i < SystemNumCores; ++i) {
		void *ptr = __aeabi_read_core_cls(i);
		printf("core %u local: %p size: %u\n", i, ptr, (size_t)&__core_data_size);
		while (ptr < __aeabi_read_core_cls(i) + (size_t)(&__core_data_size)) {
			printf("  %p: 0x%08lx\n", ptr, *(uint32_t *)ptr);
			ptr += sizeof(uint32_t);
		}
	}
	return 0;
}

static __shell_command const struct shell_command cls_dump_cmd =
{
	.name = "cls-dump",
	.usage = "",
	.func = cls_dump,
};

struct task_capture {
	size_t space;
	size_t count;
	struct task **tasks;
};

static bool count_tasks(struct sched_list *node, void *context)
{
	assert(context != 0);

	struct task_capture *capture = context;

	++capture->space;

	return true;
}

static bool capture_tasks(struct sched_list *node, void *context)
{
	struct task_capture *capture = context;

	assert(capture != 0 && capture->tasks != 0);

	capture->tasks[capture->count++] = container_of(node, struct task, scheduler_node);

	return capture->count < capture->space;
}

static int tasks(int argc, char **argv)
{
	extern struct scheduler *scheduler;
	struct task_capture capture = {.space = 0, .count = 0, .tasks = 0 };
	backtrace_t backtrace[25];
	size_t entries;

	/* Count the number of tasks and allocate space for the task pointers */
	scheduler_for_each(&scheduler->tasks, count_tasks, &capture);
	capture.tasks = alloca(sizeof(*capture.tasks) * capture.space);

	/* Now capture the task pointers */
	scheduler_for_each(&scheduler->tasks, capture_tasks, &capture);

	/* Dump the tasks, trying to detect terminations */
	for (size_t i = 0; i < capture.count; ++i) {
		struct task *task = capture.tasks[i];
		if (task->marker == SCHEDULER_TASK_MARKER) {

			/* Dump task state */
			printf("task@%p%c\n", task, task == scheduler_task() ? '*' : ' ');
			printf("  psp: %p\n", task->psp);
			printf("  tls: %p\n", task->tls);
			printf("  state: %u\n", task->state);
			printf("  core: %lu\n", task->core);
			printf("  affinity: %lu\n", task->affinity);
			printf("  base priority: %lu\n", task->base_priority);
			printf("  current priority: %lu\n", task->base_priority);
			printf("  timer expires: %lu\n", task->timer_expires);
			printf("  current queue: %p\n", task->current_queue);
			printf("  context: %p\n", task->context);
			printf("  flags: 0x%08lx\n", task->flags);

			/* Capture a backtrace of the task */
			if (task != scheduler_task()) {
				backtrace_frame_t frame = { .fp = task->psp->r7, .sp = (uint32_t)task->psp, .lr = task->psp->lr, .pc = task->psp->pc };
				entries = _backtrace_unwind(backtrace, array_sizeof(backtrace), &frame);
			} else
				entries = backtrace_unwind(backtrace, array_sizeof(backtrace));

			/* Dump the backtrace */
			printf("  backtrace:\n");
			for (size_t i = 0; i < entries - 1; ++i)
				printf("    %s@%p - %p\n", backtrace[i].name, backtrace[i].function, backtrace[i].address);
		}
	}

	return 0;
}

static __shell_command const struct shell_command task_dump_cmd =
{
	.name = "tasks",
	.usage = "",
	.func = tasks,
};
