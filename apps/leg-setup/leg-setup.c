/*
 * leg-setup.c
 *
 *  Created on: Feb 25, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdlib.h>

#include <init/init-sections.h>

#include <diag/diag.h>

#include <sys/fault.h>
#include <sys/syslog.h>

#include <rtos/rtos.h>

#include <svc/shell.h>

#include "leg-manager.h"

#define NUM_LEGS 1

struct leg_manager *leg_manager = 0;

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

thread_local uint32_t init_me = 0xdeadbeef;
thread_local uint32_t uninit_me;

int main(int argc, char **argv)
{

	uninit_me = 0xbeefdead;

	syslog_info("__tdata: %p\n", &__tdata);
	syslog_info("__tdata_size: %p\n", &__tdata_size);
	syslog_info("__tbss_size: %p\n", &__tbss_size);
	syslog_info("__tls_size: %p\n", &__tls_size);
	syslog_info("__tbss_offset: %p\n", &__tbss_offset);
	syslog_info("__tls_align: %p\n", &__tls_align);
	syslog_info("__arm32_tls_tcb_offset: %p\n", &__arm32_tls_tcb_offset);
	syslog_info("__arm64_tls_tcb_offset: %p\n", &__arm64_tls_tcb_offset);

	syslog_info("init_me %p 0x%08lx\n", &init_me, init_me);
	syslog_info("uninit_me %p 0x%08lx\n", &uninit_me, uninit_me);

	syslog_info("creating leg manager with %u legs\n", NUM_LEGS);
	leg_manager = leg_manager_create(NUM_LEGS, 57600, 10);
	if (!leg_manager) {
		syslog_error("could not create leg manager: %d\n", errno);
		return EXIT_FAILURE;
	}

	/* Wait for exit */
	syslog_info("waiting for exit\n");
	main_id = osThreadGetId();
	osThreadFlagsWait(0x7fffffff, osFlagsWaitAny, osWaitForever);

	/* Clean */
	syslog_info("cleaning up leg manager\n");
	leg_manager_destroy(leg_manager);

	return EXIT_SUCCESS;
}
