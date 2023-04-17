/*
 * leg-setup.c
 *
 *  Created on: Feb 25, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <init/init-sections.h>

#include <diag/diag.h>

#include <sys/fault.h>
#include <sys/syslog.h>

#include <rtos/rtos.h>

#include <svc/shell.h>

#include <legs/leg-manager.h>

#define NUM_LEGS 1

static osThreadId_t main_id = 0;

static int exit_cmd(int argc, char **argv)
{
	assert(argc >= 1 && argv != 0);

	if (!main_id) {
		fprintf(stderr, "main thread id is 0\n");
		return EXIT_FAILURE;
	}

	uint32_t flags = osThreadFlagsSet(main_id, 0x7fffffff);
	if (flags & osFlagsError)
		syslog_fatal("error setting main thread flags: %d\n", (osStatus_t)flags);

	return EXIT_SUCCESS;
}

static __shell_command const struct shell_command _exit_cmd =
{
	.name = "exit",
	.usage = "[value]",
	.func = exit_cmd,
};

void save_fault(const struct cortexm_fault *fault, const struct backtrace *entries, int count)
{
	for (size_t i = 0; i < count; ++i)
		diag_printf("%s@%p - %p\n", entries[i].name, entries[i].function, entries[i].address);
}

__noreturn void __assert_fail(const char *expr)
{
	diag_printf("assert failed: %s\n", expr);
	__builtin_trap();
	abort();
}

int main(int argc, char **argv)
{
	float a = M_PI;
	float b = 4096.0;
	float scale = a / b;
	unsigned int test_bits = 0x0fff;
	unsigned int lz = __builtin_clz(test_bits);
	int64_t a64 = 10;
	int64_t b64 = 42398462;
	int64_t c64 = a64 * b64;

	/* Wait for exit */
	syslog_info("waiting for exit: %f, lz: %u %lld\n", scale, lz, c64);
	main_id = osThreadGetId();
	osThreadFlagsWait(0x7fffffff, osFlagsWaitAny, osWaitForever);

	syslog_info("main exiting\n");

	return EXIT_SUCCESS;
}
