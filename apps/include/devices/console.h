/*
 * console.h
 *
 *  Created on: Feb 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <stddef.h>
#include <sys/types.h>

#include <cmsis/cmsis.h>
#include <rtos/rtos.h>

#define CONSOLE_RX 0x00000001
#define CONSOLE_TX 0x00000002

struct console
{
	UART0_Type *uart;
	IRQn_Type irq;

	osDequeId_t rx_queue;
	osDequeId_t tx_queue;

	uint32_t rx_timeout;
	uint32_t tx_timeout;

	uint32_t oe_counter;
	uint32_t pe_counter;
	uint32_t be_counter;
	uint32_t fe_counter;
};

int console_ini(struct console *console, UART0_Type* uart, IRQn_Type irq, uint32_t baud_rate, uint32_t buffer_size);
void console_fini(struct console *console);

struct console *console_create(UART0_Type* uart, IRQn_Type irq, uint32_t baud_rate, uint32_t buffer_size);
void console_destroy(struct console *console);

int console_flush_queue(struct console *console, int which);
ssize_t console_recv(struct console *console, void *buffer, size_t count, uint32_t timeout);
ssize_t console_send(struct console *console, const void *buffer, size_t count, uint32_t timeout);

#endif
