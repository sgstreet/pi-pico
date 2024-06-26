/*
 * leg-setup.c
 *
 *  Created on: Feb 25, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/fault.h>
#include <sys/syslog.h>
#include <sys/delay.h>

#include <board/board.h>
#include <rtos/rtos.h>
#include <hardware/rp2040/pio.h>
#include <hardware/rp2040/dma.h>

#include "pio-loopback.pio.h"

/*                                  1         2         3         4
 *                         1234567890123456789012345678901234567890123 4 */
static const char msg[] = "The quick brown fox jumps over the lazy dog";

static osThreadId_t tx_task_id;
static osThreadId_t rx_task_id;
static osThreadId_t monitor_task_id;
static unsigned int tx_loops = 0;
static unsigned int tx_errors = 0;
static unsigned int tx_dma_errors = 0;
static unsigned int tx_irq = 0;
static unsigned int rx_loops = 0;
static unsigned int rx_dma_errors = 0;
static unsigned int rx_errors = 0;
static unsigned int rx_irq = 0;

static void tx_dma_handler(uint32_t channel, void *context)
{
	++tx_irq;
	tx_dma_errors += dma_has_error(0) ? 1 : 0;
}

static void tx_task(void *context)
{
	uint8_t *fifo = (uint8_t *)pio_get_tx_fifo(0) + 3;

	/* Set up the tx buffer */
	char tx_msg[sizeof(msg)];
	memcpy(tx_msg, msg, sizeof(msg));

	/* Setup the tx dma channel */
	dma_clear_ctrl(0);
	dma_set_write_addr(0, (uintptr_t)fifo);
	dma_set_transfer_count(0, sizeof(tx_msg));
	dma_set_treq_sel(0, pio_get_tx_dreg(0));
	dma_set_read_increment(0, true);
	dma_set_write_increment(0, false);
	dma_set_data_size(0, sizeof(tx_msg[0]));
	dma_set_enabled(0, true);

	while (true) {

		/* Wait for the kick to to start tx dma */
		uint32_t flags = osThreadFlagsWait(0x3, osFlagsWaitAny, osWaitForever);
		if (flags & osFlagsError)
			syslog_fatal("problem waiting for kick: %d\n", (osStatus_t)flags);

		/* Start is up */
		dma_set_read_addr(0, (uintptr_t)tx_msg);
		dma_start(0);

		/* Track progress */
		++tx_loops;
	}
}

static void rx_dma_handler(uint32_t channel, void *context)
{
	++rx_irq;
	rx_dma_errors += dma_has_error(1) ? 1 : 0;

	uint32_t flags = osThreadFlagsSet(rx_task_id, 0x1);
	if (flags & osFlagsError)
		syslog_fatal("failed to kick the rx task: %d\n", (osStatus_t)flags);
}

static void rx_task(void *context)
{
	uint8_t *fifo = (uint8_t *)pio_get_rx_fifo(0) + 3;
	char rx_msg[sizeof(msg)];

	dma_clear_ctrl(1);
	dma_set_read_addr(1, (uintptr_t)fifo);
	dma_set_transfer_count(1, sizeof(rx_msg));
	dma_set_treq_sel(1, pio_get_rx_dreg(0));
	dma_set_read_increment(1, false);
	dma_set_write_increment(1, true);
	dma_set_data_size(1, sizeof(rx_msg[0]));
	dma_set_enabled(1, true);

	while (true) {

		memset(rx_msg, 0, sizeof(rx_msg));

		/* Start the rx dma channel */
		dma_set_write_addr(1, (uintptr_t)rx_msg);
		dma_start(1);

		/* Kick the transmitter */
		uint32_t flags = osThreadFlagsSet(tx_task_id, 0x1);
		if (flags & osFlagsError)
			syslog_fatal("failed to kick the transmitter: %d\n", (osStatus_t)flags);

		/* Wait for completion */
		flags = osThreadFlagsWait(0x3, osFlagsWaitAny, osWaitForever);
		if (flags & osFlagsError)
			syslog_fatal("problem waiting for rx dma to complete: %d\n", (osStatus_t)flags);

		/* Did we get what we sent? */
		if (memcmp(rx_msg, msg, sizeof(rx_msg)) != 0)
			++rx_errors;

		++rx_loops;
	}

}
static void monitor_task(void *context)
{
	/* Dump counts every second */
	syslog_info("monitoring dma results\n");
	while (true) {

		/* Wait awhile */
		osStatus_t os_status = osDelay(1000);
		if (os_status != osOK)
			syslog_fatal("failed to delay: %d", os_status);

		/* Dump the state */
		syslog_info("tx - irq: %u loops: %u errors: %u dma error: %u\n", tx_irq, tx_loops, tx_errors, tx_dma_errors);
		syslog_info("rx - irq: %u loops: %u errors: %u dma error: %u\n", rx_irq, rx_loops, rx_errors, rx_dma_errors);
	}
}

int main(int argc, char **argv)
{
	/* Load the pio loopback */
	syslog_info("loading pio loopback program\n");
	pio_load_program(0, pio_loopback_program_instructions, array_sizeof(pio_loopback_program_instructions), 0);

	/* Set the clock */
	syslog_info("setting pio %u clock to %lu\n", 0, SystemCoreClock);
	pio_machine_set_clock(0, SystemCoreClock);

	/* Unleash the pio machine */
	syslog_info("enabling pio %u\n", 0);
	pio_machine_enable(0);

	/* Loop data are run */
	syslog_info("verifying pio %u loopback program\n", 0);
	for (size_t i = 0; i < sizeof(msg); ++i) {

		*pio_get_tx_fifo(0) = msg[i];
		while (pio_rx_empty(0));
		char c = *pio_get_rx_fifo(0);
		if (c != msg[i])
			syslog_fatal("bad loopback: %hhd %hhd\n", msg[i], c);
	}
	syslog_info("pio %u loopback program good\n", 0);

	/* Register the dma channel handlers */
	syslog_info("registering dma channel interrupt handlers\n");
	dma_register_channel(0, DMA_IRQ_0_IRQn, tx_dma_handler, 0);
	dma_register_channel(1, DMA_IRQ_1_IRQn, rx_dma_handler, 0);
	dma_enable_irq(0);
	dma_enable_irq(1);

	/* Initialize the kernel */
	syslog_info("initializing the rtos kernel\n");
	osStatus_t os_status = osKernelInitialize();
	if (os_status != osOK)
		syslog_fatal("failed to initialize the kernel: %d\n", os_status);

	/* Startup the tasks */
	syslog_info("creating the tx task\n");
	osThreadAttr_t tx_task_attr = { .name = "tx-task", .attr_bits = osThreadJoinable };
	tx_task_id = osThreadNew(tx_task, 0, &tx_task_attr);
	if (!tx_task_id)
		syslog_fatal("could not create tx task: %d\n", -errno);

	syslog_info("creating the rx task\n");
	osThreadAttr_t rx_task_attr = { .name = "rx-task", .attr_bits = osThreadJoinable };
	rx_task_id = osThreadNew(rx_task, 0, &rx_task_attr);
	if (!rx_task_id)
		syslog_fatal("could not create rx task: %d\n", -errno);

	syslog_info("creating the monitor task\n");
	osThreadAttr_t monitor_task_attr = { .name = "monitor-task", .attr_bits = osThreadJoinable, .priority = osPriorityAboveNormal };
	monitor_task_id = osThreadNew(monitor_task, 0, &monitor_task_attr);
	if (!monitor_task_id)
		syslog_fatal("could not create monitor task: %d\n", -errno);

	syslog_info("starting the rtos kernel\n");
	os_status = osKernelStart();
	if (os_status != osOK)
		syslog_fatal("failed to start the rtos kernel\n");

	return EXIT_SUCCESS;
}
