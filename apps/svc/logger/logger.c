/*
 * logger.c
 *
 *  Created on: Feb 12, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <compiler.h>

#include <rtos/rtos.h>

#include <sys/syslog.h>
#include <sys/backtrace.h>
#include <sys/iob.h>

#include <diag/diag.h>
#include <devices/devices.h>

#include <svc/shell.h>

struct logger_msg
{
	size_t count;
	char data[];
};

struct logger
{
	atomic_bool run;
	struct half_duplex *output;
	osDequeId_t msg_queue;
	osMemoryPoolId_t msg_pool;
	osThreadId_t task;
	unsigned int dropped;
	struct iob stderr_iob;
};

extern const char *syslog_level_str[];
static struct logger *logger = 0;
thread_local struct logger_msg *log_msg = 0;

static int logger_put(char c, FILE *file)
{
	assert(file != 0);

	/* Untangle the interface */
	struct iob *iob = iob_from(file);
	struct logger *logger = iob->platform;

	/* Do we need a message buffer? */
	if (log_msg == 0) {
		log_msg = osMemoryPoolAlloc(logger->msg_pool, 0);
		if (!log_msg) {
			++logger->dropped;
			return EOF;
		}
		log_msg->count = 0;
	}

	/* Add the character to the message buffer */
	log_msg->data[log_msg->count++] = c;

	/* Should we flush the buffer? */
	if (log_msg->count == (LOGGER_MSG_SIZE - sizeof(struct logger_msg)) || c == '\n') {
		int status = iob->file_close.file.flush(file);
		if (status < 0)
			return EOF;
	}

	/* All done */
	return c;
}

static int logger_flush(FILE *file)
{
	assert(file != 0);

	/* Ignore if there is no buffer */
	if (!log_msg)
		return 0;

	/* Untangle the interface */
	struct iob *iob = iob_from(file);
	struct logger *logger = iob->platform;

	/* Post to the the logger task */
	osStatus_t os_status = osDequePutBack(logger->msg_queue, &log_msg, 0);
	if (os_status != osOK) {
		++logger->dropped;
		log_msg->count = 0;
		return EOF;
	}

	/* Clear the thread local */
	log_msg = 0;

	/* All good */
	return 0;
}

static int logger_close(FILE *file)
{
	assert(file != 0);

	struct iob *iob = iob_from(file);

	return iob->file_close.file.flush(file);
}

static void logger_task(void *context)
{
	assert(context != 0);

	struct logger_msg *msg;
	struct logger *logger = context;

	/* While still running */
	while (logger->run) {

		/* Wait for msg */
		osStatus_t os_status = osDequeGetFront(logger->msg_queue, &msg, osWaitForever);
		if (os_status != osOK) {
			if (os_status == osErrorResource)
				continue;
			syslog_fatal("unknown failure getting message from queue: %d\n", os_status);
		}

		/* Send to the output channel */
		half_duplex_send(logger->output, msg->data, msg->count);

		/* Release the message */
		os_status = osMemoryPoolFree(logger->msg_pool, msg);
		if (os_status != osOK)
			syslog_fatal("failure releasing message to pool: %d\n", os_status);
	}

	/* Restore the old error iob */
	_stderr = logger->stderr_iob;
	syslog_info("logger exiting\n");
}

static __constructor_priority(SERVICES_PRIORITY) void logger_ini(void)
{
	/* Get some memory for the logger */
	struct logger *new_logger = calloc(1, sizeof(struct logger));
	if (!new_logger)
		syslog_fatal("could not allocate memory for logger\n");

	/* Create the message queue */
	new_logger->msg_queue = osDequeNew(LOGGER_QUEUE_SIZE, sizeof(char *), 0);
	if (!new_logger->msg_queue)
		syslog_fatal("could create new message deque: %d\n", errno);

	/* Create the msg pool */
	new_logger->msg_pool = osMemoryPoolNew(LOGGER_QUEUE_SIZE, LOGGER_MSG_SIZE, 0);
	if (!new_logger->msg_pool)
		syslog_fatal("could create new message pool: %d\n", errno);

	/* Get the half duplex channel */
	new_logger->output = device_get_half_duplex_channel(LOGGER_HALF_DUPLEX_CHANNEL);
	if (!new_logger->output)
		syslog_fatal("could get half duplex channel: %lu\n", LOGGER_HALF_DUPLEX_CHANNEL);

	/* Save the old stderr iob */
	new_logger->stderr_iob = _stderr;

	/* Initialize logger and bind to to stddiag for logging output */
	logger = new_logger;
	iob_setup_stream(&_stddiag, logger_put, 0, logger_flush, logger_close, __SWR , logger);

	/* Start up the logging task */
	new_logger->run = true;
	osThreadAttr_t task_attr = { .name = "logger-task", .attr_bits = osThreadJoinable, .stack_size = LOGGER_STACK_SIZE, .priority = osPriorityNormal };
	logger->task = osThreadNew(logger_task, new_logger, &task_attr);
	if (!logger->task)
		syslog_fatal("could not create logger task\n");
}

static __destructor_priority(SERVICES_PRIORITY) void logger_fini(void)
{
	/* Skip if logger has not been initialized */
	if (!logger)
		return;

	/* Mark for exit */
	struct logger *old_logger = logger;
	old_logger->run = false;

	/* Rest the message queue to force the task to unblock */
	osStatus_t os_status = osDequeReset(old_logger->msg_queue);
	if (os_status != osOK)
		syslog_fatal("failed to reset the message queue: %d", os_status);

	/* Wait the task to exit */
	os_status = osThreadJoin(old_logger->task);
	if (os_status != osOK)
		syslog_fatal("failed to join with the logger task: %d\n", os_status);

	/* Delete the message queue */
	os_status = osDequeDelete(old_logger->msg_queue);
	if (os_status != osOK)
		syslog_fatal("failed to delete the message queue: %d\n", os_status);

	/* Delete the message pool */
	os_status = osMemoryPoolDelete(old_logger->msg_pool);
	if (os_status != osOK)
		syslog_fatal("failed to delete the message pool: %d\n", os_status);

	/* Release the logger */
	free(old_logger);
}

static int log_main(int argc, char **argv)
{
	char c ;
	int opt_index = 0;
	unsigned int level = SYSLOG_NONE;

	struct option long_options[] =
	{
		{
			.name = "level",
			.has_arg = required_argument,
			.flag = 0,
			.val = 'l',
		},
	};

	optind = 0;
	while ((c = getopt_long(argc, argv, ":l:", long_options, &opt_index)) != -1) {

		switch (c) {
			case 'l': {
				errno = 0;
				char *end;
				level = strtoul(optarg, &end, 0);
				if (errno != 0 || end == optarg || level > SYSLOG_TRACE) {
					printf("bad level: '%s'\n", optarg);
					return EXIT_FAILURE;
				}
				break;
			}
			default: {
				printf("unknown options\n");
				return EXIT_FAILURE;
			}
		}
	}

	if (optind < argc) {
		fprintf(stddiag, "%s log - ", syslog_level_str[level]);
		while (optind < argc)
			fprintf(stddiag, "%s ", argv[optind++]);
		fprintf(stddiag, "\n");
	}

	/* All good */
	return EXIT_SUCCESS;
}

static __shell_command const struct shell_command log_cmd =
{
	.name = "log",
	.usage = "[-l,--level <number>] <the message to log>",
	.func = log_main,
};

