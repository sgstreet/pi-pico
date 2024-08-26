/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  Copyright: @2024 Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <unistd.h>

#include <compiler.h>

#include <sys/syslog.h>

#include <rtos/rtos.h>

#include <svc/shell.h>
#include <devices/cdc-serial.h>

static osOnceFlag_t cdc_serial_echo_atexit_flag = osOnceFlagsInit;
static osThreadId_t cdc_serial_echo_id = 0;
static atomic_bool run = false;

static void cdc_serial_echo_atexit(void)
{
	/* Terminate and join if the task is running */
	if (cdc_serial_echo_id != 0) {
		run = false;
		osStatus_t os_status = osThreadJoin(cdc_serial_echo_id);
		if (os_status != osOK)
			syslog_fatal("failed to joined with %p: %d\n", cdc_serial_echo_id, os_status);
		cdc_serial_echo_id = 0;
	}
}

static void cdc_serial_echo_atexit_init(osOnceFlagId_t flag_id, void *context)
{
	atexit(cdc_serial_echo_atexit);
}

static void cdc_serial_echo_task(void *context)
{
	char buffer[128];

	syslog_info("started\n");

	osCallOnce(&cdc_serial_echo_atexit_flag, cdc_serial_echo_atexit_init, 0);

	int fd = open("ACM1", O_RDWR);
	if (fd < 0)
		syslog_fatal("failed to open the ACM1: %d\n", errno);

	while (run) {

		ssize_t amount = read(fd, buffer, sizeof(buffer));
		if (amount < 0)
			syslog_fatal("cdc serial read failed: %d\n", errno);

		struct cdc_serial *serial = cdc_serial_from_fd(fd);

		syslog_info("tx ring - r: %lu w: %lu: i: %lu r+: %d w+: %d space: %u avail: %u\n", serial->ring.tx_buffer.read_index, serial->ring.tx_buffer.write_index, serial->ring.tx_buffer.invalidate_index, serial->ring.tx_buffer.read_wrapped, serial->ring.tx_buffer.write_wrapped, bip_buffer_space_available(&serial->ring.tx_buffer), bip_buffer_data_available(&serial->ring.tx_buffer));

		strcpy(buffer, "Received 1 char 0123456789 0123456789 0123456789 0123456789 0123456789\n");

		ssize_t count = strlen(buffer);\
		amount = 0;
		while (amount < count) {
			ssize_t written = write(fd, buffer + amount, count - amount);
			if (written < 0)
				syslog_fatal("failed to write %d out of %d bytes\n", count - amount, count);
			amount += written;
		}

//		char *data = buffer;
//		while (amount > 0) {
//			size_t sent = write(fd, data, amount);
//			if (sent < 0)
//				syslog_fatal("cdc serial write failed: %d\n", errno);
//			amount -= sent;
//			data += sent;
//		}
	}

	close(fd);

	syslog_info("stopped\n");
}

static int cdc_serial_cmd(int argc, char **argv)
{
	assert(argc >= 1 && argv != 0);

	if (argc <= 1) {
		printf("cdc serial echo is %srunning\n", cdc_serial_echo_id == 0 ? "not " : "");
		return EXIT_SUCCESS;
	}

	/* Execute required action */
	if (strcasecmp(argv[1], "start") == 0) {

		/* Already running? */
		if (cdc_serial_echo_id != 0)
			return EXIT_SUCCESS;

		/* Start up the echo task */
		run = true;
		osThreadAttr_t thread_attr = { .name = "cdc-serial-echo", .stack_size = 2048, .attr_bits = osThreadJoinable };
		cdc_serial_echo_id = osThreadNew(cdc_serial_echo_task, 0, &thread_attr);
		if (!cdc_serial_echo_id)
			syslog_fatal("failed to start the cdc serial echo task: %d\n", errno);

	} else if (strcasecmp(argv[1], "stop") == 0) {

		/* Already stopped? */
		if (cdc_serial_echo_id == 0)
			return EXIT_SUCCESS;

		/* Terminated the task */
		run = false;
		osStatus_t os_status = osThreadJoin(cdc_serial_echo_id);
		if (os_status != osOK)
			syslog_fatal("failed to joined with %p: %d\n", cdc_serial_echo_id, os_status);
		cdc_serial_echo_id = 0;

	} else {
		fprintf(stderr, "unknown argument\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static __shell_command const struct shell_command _exit_cmd =
{
	.name = "cdc-serial",
	.usage = "(start | stop)",
	.func = cdc_serial_cmd,
};

static __constructor void cdc_serial_echo_ini(void)
{
}

static __destructor void cdc_serial_echo_fini(void)
{
}
