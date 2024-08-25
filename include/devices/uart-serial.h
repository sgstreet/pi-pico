/*
 * uart-serial.h
 *
 *  Created on: Aug 17, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _UART_SERIAL_H_
#define _UART_SERIAL_H_

#include <config.h>
#include <container-of.h>
#include <ref.h>

#include <devices/io-ring.h>
#include <devices/wait-queue.h>

#include <cmsis/cmsis.h>
#include <devices/posix-io.h>

#ifndef UART_BAUD_RATE
#define UART_BAUD_RATE 115200UL
#endif

#ifndef UART_BUFFER_SIZE
#define UART_BUFFER_SIZE 1024UL
#endif

#define UART_SERIAL_RX 0x00000001
#define UART_SERIAL_TX 0x00000002

struct uart_serial
{
	unsigned int channel;
	UART0_Type *uart;
	IRQn_Type irq;

	struct io_ring ring;
	struct wait_queue data_available;
	struct wait_queue space_available;
	struct ref ref;

	uint32_t rx_timeout;
	uint32_t tx_timeout;

	uint32_t rd_counter;
	uint32_t oe_counter;
	uint32_t pe_counter;
	uint32_t be_counter;
	uint32_t fe_counter;
};

struct uart_serial_file
{
	struct uart_serial *device;
	struct posix_ops ops;
};

int uart_serial_ini(struct uart_serial *serial, UART0_Type* uart, IRQn_Type irq, uint32_t baud_rate, uint32_t buffer_size);
void uart_serial_fini(struct uart_serial *serial);

struct uart_serial *uart_serial_create(UART0_Type* uart, IRQn_Type irq, uint32_t baud_rate, uint32_t buffer_size);
void uart_serial_destroy(struct uart_serial *serial);

int uart_serial_flush_queue(struct uart_serial *serial, int which);
ssize_t uart_serial_recv(struct uart_serial *serial, void *buffer, size_t count, uint32_t timeout);
ssize_t uart_serial_send(struct uart_serial *serial, const void *buffer, size_t count, uint32_t timeout);

static inline struct uart_serial *uart_serial_from_fd(int fd)
{
	struct uart_serial_file *file = container_of_or_null((struct posix_ops *)fd, struct uart_serial_file, ops);
	return file ? file->device : 0;
}


#endif
