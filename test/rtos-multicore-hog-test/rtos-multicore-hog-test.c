/*
 * Copyright (C) 2024 Stephen Street
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * rtos-hog-test.c
 *
 *  Created on: Mar 14, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/tls.h>
#include <cmsis/cmsis.h>

#include <rtos/rtos-toolkit/scheduler.h>

#define NUM_HOGS 8

struct hog
{
	struct task *id;
	unsigned int loops;
	unsigned int cores[SystemNumCores];
};

int picolibc_putc(char c, FILE *file);
int picolibc_getc(FILE *file);

struct hog hogs[NUM_HOGS] = { 0 };
unsigned int kick_counter = 0;
unsigned int wake_cores[SystemNumCores] = { 0, 0 };
struct task *wake_counter_id = 0;
struct task *dump_task_id = 0;
long events = 0;
struct futex futex;

static void wake_counter_task(void *context)
{
	while (true) {
		int status = scheduler_futex_wait(&futex, 0, SCHEDULER_WAIT_FOREVER);
		if (status < 0) {
			printf("failed to wait for futex: %d\n", status);
			abort();
		}
		++wake_cores[SystemCurrentCore];
	}
}

static void hog_task(void *context)
{
	struct hog *hog = context;

	while (true) {
		unsigned int core = SystemCurrentCore;
		++hog->loops;
		++hog->cores[core];
		if ((random() & 0x8) == 0) {
			scheduler_futex_wake(&futex, true);
			++kick_counter;
			scheduler_yield();
		}
	}
}

static void dump_task(void *context)
{
	/* Dump hog info */
	int counter = 0;
	while (true) {
		printf("--- %d\n", counter++);
		printf("\twake = [%u, %u, %u]\n", kick_counter, wake_cores[0], wake_cores[1]);
		for (int i = 0; i < NUM_HOGS; ++i)
			printf("\thog[%d] = [%u, %u]\n", i, hogs[i].cores[0], hogs[i].cores[1]);
		scheduler_sleep(1000);
	}
}

int main(int argc, char **argv)
{
	struct scheduler scheduler;

	int status = scheduler_init(&scheduler, _tls_size());
	if (status < 0) {
		printf("failed to initialize the scheduler\n");
		abort();
	}

	scheduler_futex_init(&futex, &events, 0);

	struct task_descriptor dump_task_desc = { .entry_point = dump_task, .context = 0, .priority = SCHEDULER_MAX_TASK_PRIORITY };
	dump_task_id = scheduler_create(sbrk(1024), 1024, &dump_task_desc);
	if (!dump_task_id) {
		printf("failed to start dump_task\n");
		abort();
	}

	struct task_descriptor wake_counter_task_desc = { .entry_point = wake_counter_task, .context = &futex, .priority = SCHEDULER_MIN_TASK_PRIORITY / 4 };
	wake_counter_id = scheduler_create(sbrk(1024), 1024, &wake_counter_task_desc);
	if (!wake_counter_id) {
		printf("failed to start wake_counter_task\n");
		abort();
	}

	for (int i = 0; i < NUM_HOGS; ++i) {
		struct task_descriptor hog_task_desc = { .entry_point = hog_task, .context = &hogs[i], .priority = SCHEDULER_MIN_TASK_PRIORITY / 2 };
		if (i < 2) {
			hog_task_desc.flags |= SCHEDULER_CORE_AFFINITY;
			hog_task_desc.affinity = i;
		}
		hogs[i].id = scheduler_create(sbrk(1024), 1024, &hog_task_desc);
		if (!hogs[i].id) {
			printf("failed to start hog %d\n", i);
			abort();
		}
	}

	/* Run the scheduler */
	scheduler_run();

	/* We will never reach here */
	return EXIT_SUCCESS;
}
