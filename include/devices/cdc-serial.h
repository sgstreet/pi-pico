/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  Copyright: @2024 Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _CDC_SERIAL_H_
#define _CDC_SERIAL_H_

#include <container-of.h>
#include <ref.h>

#include <devices/io-ring.h>
#include <devices/wait-queue.h>
#include <devices/posix-io.h>
#include <rtos/rtos.h>

struct serial_line_coding
{
	unsigned int baud_rate;
	unsigned char data_bits;
	unsigned char stops_bits;
	char parity;
};

struct serial_line_state
{
	bool dtr;
	bool rts;
};

struct cdc_serial
{
	unsigned int channel;
	struct serial_line_coding line_coding;
	struct serial_line_state line_state;
	struct io_ring ring;
	osMutexId_t tx_lock;
	osMutexId_t rx_lock;
	osTimerId_t tx_flush_timer;
	struct wait_queue data_available;
	struct wait_queue space_available;
	int usbd_fd;
	struct ref ref;
};

struct cdc_serial_file
{
	struct cdc_serial *device;
	struct posix_ops ops;
};

struct serial_line_coding cdc_serial_get_line_coding(const struct cdc_serial *serial);
struct serial_line_state cdc_serial_get_line_state(const struct cdc_serial *serial);
unsigned int cdc_serial_get_channel(const struct cdc_serial *serial);

ssize_t cdc_serial_recv(struct cdc_serial *serial, void *buffer, size_t count, unsigned int msecs);
ssize_t cdc_serial_send(struct cdc_serial *serial, const void *buffer, size_t count, unsigned int msecs);

int cdc_serial_ini(struct cdc_serial *serial, unsigned int channel);
void cdc_serial_fini(struct cdc_serial *serial);

struct cdc_serial *cdc_serial_create(unsigned channel);
void cdc_serial_destroy(struct cdc_serial *serial);

static inline struct cdc_serial *cdc_serial_from_fd(int fd)
{
	struct cdc_serial_file *file = container_of_or_null((struct posix_ops *)fd, struct cdc_serial_file, ops);
	return file ? file->device : 0;
}

#endif
