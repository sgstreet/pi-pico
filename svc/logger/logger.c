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
#include <hardware/hardware.h>

#include <svc/shell.h>

#ifndef LOGGEER_MSG_SIZE
#define LOGGER_MSG_SIZE 128UL
#endif

#ifndef LOGGER_QUEUE_SIZE
#define LOGGER_QUEUE_SIZE 32
#endif

#ifndef LOGGER_STACK_SIZE
#define LOGGER_STACK_SIZE 1024
#endif

struct logger_msg
{
	size_t count;
	char data[];
};

struct logger
{
	atomic_bool run;
	osDequeId_t msg_queue;
	osMemoryPoolId_t msg_pool;
	osThreadId_t task;
	unsigned int dropped;
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

static void logger_tx_done(uint32_t channel, void *context)
{
	assert(channel == BOARD_LOGGER_DMA_CHANNEL && context != 0);

	osThreadId_t logger_id = context;

	uint32_t events = osThreadFlagsSet(logger_id, RTOS_IO_DONE);
	if (events & osFlagsError)
		syslog_fatal("failed to notify logger of IO completions: %d\n", (osStatus_t)events);
}

static void logger_task(void *context)
{
	assert(context != 0);

	struct logger_msg *msg;
	struct logger *logger = context;
	struct iob stddiag_iob = _stddiag;

	/* Register with the dma engine */
	dma_register_channel(BOARD_LOGGER_DMA_CHANNEL, DMA_IRQ_0_IRQn, logger_tx_done, osThreadGetId());

	/* Initialize the DMA channel */
	dma_clear_ctrl(BOARD_LOGGER_DMA_CHANNEL);
	dma_set_write_addr(BOARD_LOGGER_DMA_CHANNEL, (uintptr_t)&BOARD_LOGGER_UART->UARTDR);
	dma_set_treq_sel(BOARD_LOGGER_DMA_CHANNEL, BOARD_LOGGER_DMA_DREQ);
	dma_set_read_increment(BOARD_LOGGER_DMA_CHANNEL, true);
	dma_set_write_increment(BOARD_LOGGER_DMA_CHANNEL, false);
	dma_set_data_size(BOARD_LOGGER_DMA_CHANNEL, sizeof(char));
	dma_set_enabled(BOARD_LOGGER_DMA_CHANNEL, true);
	dma_enable_irq(BOARD_LOGGER_DMA_CHANNEL);

	/* Make sure the UART DREQ is enabled */
	set_bit(&BOARD_LOGGER_UART->UARTDMACR, UART0_UARTDMACR_TXDMAE_Pos);

	/* Initialize logger and bind to to stddiag for logging output */
	iob_setup_stream(&_stddiag, logger_put, 0, logger_flush, logger_close, __SWR , logger);

	syslog_info("ready\n");

	/* While still running */
	while (logger->run) {

		/* Wait for msg */
		osStatus_t os_status = osDequeGetFront(logger->msg_queue, &msg, osWaitForever);
		if (os_status != osOK) {
			if (os_status == osErrorResource)
				continue;
			syslog_fatal("unknown failure getting message from queue: %d\n", os_status);
		}

		/* Make sure the UART DREQ is enabled, this is require as some might have call the board_putc and friends */
		set_bit(&BOARD_LOGGER_UART->UARTDMACR, UART0_UARTDMACR_TXDMAE_Pos);

		/* Launch the DMA channel */
		dma_set_read_addr(BOARD_LOGGER_DMA_CHANNEL, (uintptr_t)msg->data);
		dma_set_transfer_count(BOARD_LOGGER_DMA_CHANNEL, msg->count);
		dma_start(BOARD_LOGGER_DMA_CHANNEL);

		/* Wait for dma to complete */
		uint32_t events = osThreadFlagsWait(RTOS_IO_DONE, osFlagsWaitAny, osWaitForever);
		if (events & osFlagsError)
			syslog_fatal("failed to wait for dma completion event: %d\n", (osStatus_t)events);

		/* Release the message */
		os_status = osMemoryPoolFree(logger->msg_pool, msg);
		if (os_status != osOK)
			syslog_fatal("failure releasing message to pool: %d\n", os_status);
	}

	syslog_info("exiting\n");

	/* Restore the old error iob */
	_stddiag = stddiag_iob;

	/* Unregister with the dma engine */
	dma_unregister_channel(BOARD_LOGGER_DMA_CHANNEL);
}

static __constructor_priority(SERVICES_LOGGER_PRIORITY) void logger_ini(void)
{
	/* Get some memory for the logger */
	struct logger *logger = calloc(1, sizeof(struct logger));
	if (!logger)
		syslog_fatal("could not allocate memory for logger\n");

	/* Create the message queue */
	logger->msg_queue = osDequeNew(LOGGER_QUEUE_SIZE, sizeof(char *), 0);
	if (!logger->msg_queue)
		syslog_fatal("could create new message deque: %d\n", errno);

	/* Create the msg pool */
	logger->msg_pool = osMemoryPoolNew(LOGGER_QUEUE_SIZE, LOGGER_MSG_SIZE, 0);
	if (!logger->msg_pool)
		syslog_fatal("could create new message pool: %d\n", errno);

	/* Start up the logging task */
	logger->run = true;
	osThreadAttr_t task_attr = { .name = "logger-task", .attr_bits = osThreadJoinable, .stack_size = LOGGER_STACK_SIZE, .priority = osPriorityNormal };
	logger->task = osThreadNew(logger_task, logger, &task_attr);
	if (!logger->task)
		syslog_fatal("could not create logger task\n");
}

static __destructor_priority(SERVICES_LOGGER_PRIORITY) void logger_fini(void)
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

	syslog_info("destroyed\n");
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

