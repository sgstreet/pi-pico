/*
 * uart-serial.c
 *
 *  Created on: Aug 17, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <sys/syslog.h>
#include <sys/irq.h>

#include <devices/uart-serial.h>

static osOnceFlag_t device_init_flags[BOARD_NUM_UARTS] = { [0 ... BOARD_NUM_UARTS - 1] = osOnceFlagsInit };
static struct uart_serial *devices[BOARD_NUM_UARTS] = { [0 ... BOARD_NUM_UARTS - 1] = 0 };

static void uart_serial_fill_tx(struct uart_serial *serial)
{
	assert(serial != 0);

	/* Loop trying to send all the data? */
	while (io_ring_data_available(io_ring_get_device(&serial->ring))) {

		/* Get a pointer to the available data */
		size_t avail = SIZE_MAX;
		char *buffer = io_ring_read_acquire(io_ring_get_device(&serial->ring), &avail);

		/* Add the the fifo */
		size_t amount = 0;
		while ((serial->uart->UARTFR & UART0_UARTFR_TXFF_Msk) == 0 && amount < avail)
			serial->uart->UARTDR = buffer[amount++];

		/* Release the amount added to the fifo */
		io_ring_read_release(io_ring_get_device(&serial->ring), amount);
	}
}

static void uart_serial_handler(IRQn_Type irq, void *context)
{
	assert(context != 0);

	struct uart_serial *serial = context;

	/* Get a pointer to the available space */
	size_t avail = SIZE_MAX;
	char *buffer = io_ring_write_acquire(io_ring_get_device(&serial->ring), &avail);

	/* Save and clear the interrupt status register */
	uint32_t int_status = serial->uart->UARTMIS;
	serial->uart->UARTICR = int_status;

	/* Should we empty the RX FIFO? */
	size_t amount = 0;
	while ((serial->uart->UARTFR & UART0_UARTFR_RXFE_Msk) == 0 && amount < avail) {
		uint32_t c = serial->uart->UARTDR;
		if ((c & (UART0_UARTDR_OE_Msk | UART0_UARTDR_BE_Msk | UART0_UARTDR_PE_Msk | UART0_UARTDR_FE_Msk)) != 0) {
			serial->oe_counter += (c & UART0_UARTDR_OE_Msk) >> UART0_UARTDR_OE_Pos;
			serial->be_counter += (c & UART0_UARTDR_BE_Msk) >> UART0_UARTDR_BE_Pos;
			serial->pe_counter += (c & UART0_UARTDR_PE_Msk) >> UART0_UARTDR_PE_Pos;
			serial->fe_counter += (c & UART0_UARTDR_FE_Msk) >> UART0_UARTDR_FE_Pos;
			continue;
		}

		/* Save the data in the ring */
		buffer[amount++] = c;
	}

	/* Release the amount write to the uart */
	io_ring_write_release(io_ring_get_device(&serial->ring), amount);

	/* Fill up the tx fifo,  */
	uart_serial_fill_tx(serial);
}

static void uart_host_handler(struct io_interface *interface, enum io_ring_event event, size_t amount, void *context)
{
	assert(interface != 0 && context != 0);

	struct uart_serial *serial = context;

	if (event == IO_RING_SPACE_AVAIL && amount != 0)
		wait_notify(&serial->space_available, false);

	if (event == IO_RING_DATA_AVAIL && amount != 0)
		wait_notify(&serial->data_available, false);
}

static void uart_device_handler(struct io_interface *interface, enum io_ring_event event, size_t amount, void *context)
{
	assert(interface != 0 && context != 0);

	struct uart_serial *serial = context;

	if (event == IO_RING_DATA_AVAIL && amount != 0)
		uart_serial_fill_tx(serial);
}

int uart_serial_ini(struct uart_serial *serial, UART0_Type* uart, IRQn_Type irq, uint32_t baud_rate, uint32_t buffer_size)
{
	assert(serial != 0 && uart != 0);

	/* Initialize the io ring */
	int status = io_ring_ini(&serial->ring, 0, UART_BUFFER_SIZE);
	if (status < 0)
		syslog_fatal("failed to initialize the io ring: %d\n", errno);

	/* Bind the host io ring handlers */
	io_ring_set_callback(io_ring_get_host(&serial->ring), uart_host_handler, serial);
	io_ring_set_callback(io_ring_get_device(&serial->ring), uart_device_handler, serial);

	/* Initialize the wait queues */
	wait_queue_ini(&serial->data_available);
	wait_queue_ini(&serial->space_available);

	/* Save the uart and irq */
	serial->uart = uart;
	serial->irq = irq;

	/* Set up the baud rate */
    uint32_t baud_rate_div = ((BOARD_CLOCK_PERI_HZ * 8) / baud_rate);
	serial->uart->UARTIBRD = baud_rate_div >> 7;
	serial->uart->UARTFBRD = ((baud_rate_div & 0x7f) + 1) / 2;

	/* 8N1 and enable the fifos */
	serial->uart->UARTLCR_H = (3UL << UART0_UARTLCR_H_WLEN_Pos) | UART0_UARTLCR_H_FEN_Msk;

	/* Clear the rx error register */
	serial->uart->UARTRSR = UART0_UARTRSR_OE_Msk | UART0_UARTRSR_BE_Msk | UART0_UARTRSR_PE_Msk | UART0_UARTRSR_FE_Msk;

	/* Set the FIFO levels */
	serial->uart->UARTIFLS = (0x2 << UART0_UARTIMSC_RXIM_Pos) | (0x2 << UART0_UARTIMSC_TXIM_Pos);

	/* Enable the uart */
	serial->uart->UARTCR = UART0_UARTCR_UARTEN_Msk | UART0_UARTCR_TXE_Msk | UART0_UARTCR_RXE_Msk;

	/* Register and enable the interrupt handler */
	irq_register(serial->irq, INTERRUPT_NORMAL, uart_serial_handler, serial);
	irq_enable(serial->irq);

	/* Enable the uart interrupts */
	serial->uart->UARTIMSC |= (UART0_UARTIMSC_RTIM_Msk | UART0_UARTIMSC_TXIM_Msk | UART0_UARTIMSC_RXIM_Msk);

	/* All good */
	return 0;
}

void uart_serial_fini(struct uart_serial *serial)
{
	assert(serial != 0);

	/* Disable the uart */
	serial->uart->UARTCR &= ~(UART0_UARTCR_UARTEN_Msk | UART0_UARTCR_TXE_Msk | UART0_UARTCR_RXE_Msk);

	/* Clean up the interrupt handler */
	irq_unregister(serial->irq, uart_serial_handler);

	/* Release the wait queues */
	wait_queue_fini(&serial->space_available);
	wait_queue_fini(&serial->data_available);

	/* Release the io rings */
	io_ring_fini(&serial->ring);
}

struct uart_serial *uart_serial_create(UART0_Type* uart, IRQn_Type irq, uint32_t baud_rate, uint32_t buffer_size)
{
	/* Allocate the uart serial device */
	struct uart_serial *serial = malloc(sizeof(struct uart_serial));
	if (!serial) {
		errno = ENOMEM;
		return 0;
	}

	/* Forward */
	int status = uart_serial_ini(serial, uart, irq, baud_rate, buffer_size);
	if (status < 0) {
		free(serial);
		return 0;
	}

	/* All done */
	return serial;
}

void uart_serial_destroy(struct uart_serial *serial)
{
	assert(serial != 0);

	/* Forward to clean up */
	uart_serial_fini(serial);

	/* Release the memory */
	free(serial);
}

int uart_serial_flush_queue(struct uart_serial *serial, int which)
{
	assert(serial != 0);
	errno = ENOTSUP;
	return -ENOTSUP;
}

ssize_t uart_serial_recv(struct uart_serial *serial, void *buffer, size_t count, uint32_t timeout)
{
	assert(serial != 0 && buffer != 0);

	int status;
	ssize_t amount = 0;
	struct io_interface *interface = io_ring_get_host(&serial->ring);

	/* Block until we read some data */
	while (amount == 0) {

		/* Wait for data */
		if (timeout != osWaitForever) {
			status = wait_event_timeout(&serial->data_available, io_ring_data_available(interface), timeout);
			if (status <= 0)
				return status;
			timeout -= status;
		} else {
			status = wait_event(&serial->data_available, io_ring_data_available(interface));
			if (status <= 0)
				return status;
		}

		/* Read all data up to the count */
		amount = io_ring_read(interface, buffer, count);
	}

	/* Return the amount read */
	return amount;
}

ssize_t uart_serial_send(struct uart_serial *serial, const void *buffer, size_t count, uint32_t timeout)
{
	assert(serial != 0 && buffer != 0);

	int status;
	ssize_t amount = 0;
	struct io_interface *interface = io_ring_get_host(&serial->ring);

	/* Block until some data sent */
	while (amount == 0) {

		/* Wait for space */
		if (timeout != osWaitForever) {
			status = wait_event_timeout(&serial->space_available, io_ring_space_available(interface), timeout);
			if (status < 0)
				return status;
			timeout -= status;
		} else {
			status = wait_event(&serial->space_available, io_ring_space_available(interface));
			if (status < 0)
				return status;
		}

		/* Send all data up to the count */
		amount = io_ring_write(interface, buffer, count);
	}

	/* Return the amount written */
	return amount;
}

static void uart_device_release(struct ref *ref)
{
	assert(ref != 0);

	/* Get the underlay serial device */
	struct uart_serial *device = container_of(ref, struct uart_serial, ref);

	/* Clear the device entry */
	devices[device->channel] = 0;
	device_init_flags[device->channel] = osOnceFlagsInit;

	/* Forward to the destructor */
	uart_serial_destroy(device);
}

static void uart_device_init(osOnceFlagId_t flag_id, void *context)
{
	unsigned int channel = (unsigned int)context;

	/* Try to create the device */
	devices[channel] = uart_serial_create(board_uarts[channel].uart, board_uarts[channel].irq, UART_BAUD_RATE, UART_BUFFER_SIZE);
	if (!devices[channel])
		syslog_fatal("failed to create UART%u: %d\n", channel, errno);

	/* Save the channel */
	devices[channel]->channel = channel;

	/* Initialize the reference count */
	ref_ini(&devices[channel]->ref, uart_device_release, 0);
}

static struct uart_serial *uart_serial_get(unsigned int channel)
{
	assert(channel < BOARD_NUM_UARTS);

	/* Setup the matching uart serial device */
	osCallOnce(&device_init_flags[channel], uart_device_init, (void *)channel);

	ref_up(&devices[channel]->ref);

	return devices[channel];
}

static void uart_serial_put(struct uart_serial *serial)
{
	assert(serial != 0);

	ref_down(&serial->ref);
}

static int uart_serial_posix_close(int fd)
{
	assert(fd > 0);

	struct uart_serial_file *file = container_of((struct posix_ops *)fd, struct uart_serial_file, ops);

	/* Release reference to the device */
	uart_serial_put(file->device);

	/* Release the uart serial file */
	free(file);

	/* Always good */
	return 0;
}

static ssize_t uart_serial_posix_read(int fd, void *buf, size_t count)
{
	return uart_serial_recv(uart_serial_from_fd(fd), buf, count, osWaitForever);
}

static ssize_t uart_serial_posix_write(int fd, const void *buf, size_t count)
{
	return uart_serial_send(uart_serial_from_fd(fd), buf, count, osWaitForever);
}

static off_t uart_serial_posix_lseek(int fd, off_t offset, int whence)
{
	errno = ENOTSUP;
	return (off_t)-1;
}

static int uart_serial_open(const char *path, int flags)
{
	/* Check the path */
	if (path == 0 || strlen(path) < strlen("UART")) {
		errno = EINVAL;
		return -1;
	}

	/* Extract the channel index */
	errno = 0;
	char *end;
	unsigned int channel = strtoul(path + 4, &end, 0);
	if (errno != 0 || end == path + 3 || channel >= BOARD_NUM_UARTS) {
		errno = ENODEV;
		return -1;
	}

	/* Allocate the uart serial channel */
	struct uart_serial_file *file = calloc(1, sizeof(struct uart_serial_file));
	if (!file) {
		errno = ENOMEM;
		return -ENOMEM;
	}

	/* Get the underlaying uart device */
	file->device = uart_serial_get(channel);

	/* Setup the posix interface */
	file->ops.close = uart_serial_posix_close;
	file->ops.read = uart_serial_posix_read;
	file->ops.write = uart_serial_posix_write;
	file->ops.lseek = uart_serial_posix_lseek;

	/* Return the ops as a integer */
	return (intptr_t)&file->ops;
}

static __constructor_priority(DEVICES_PRIORITY) void uart_serial_posix_ini(void)
{
	int status = posix_register("UART", uart_serial_open);
	if (status != 0)
		syslog_fatal("failed to register posix device: %d\n", errno);
}

static __destructor_priority(DEVICES_PRIORITY) void uart_serial_posix_fini(void)
{
	posix_unregister("UART");
}

