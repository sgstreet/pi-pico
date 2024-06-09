/*
 * io-ring.h
 *
 *  Created on: Dec 2, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _IO_RING_H_
#define _IO_RING_H_

#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>

#include <bip-buffer.h>

enum io_ring_event
{
	IO_RING_DATA_AVAIL = 0,
	IO_RING_SPACE_AVAIL,
	IO_RING_DONE,
	IO_RING_ERROR,
	IO_RING_CANCEL,
};

struct io_interface;
struct io_ring;

typedef void (*io_interface_callback_t)(struct io_interface *interface, enum io_ring_event event, size_t amount, void *context);

struct io_interface
{
	struct bip_buffer *rx;
	struct bip_buffer *tx;
	io_interface_callback_t *callback;
	void **context;
	struct io_ring *parent;
};

struct io_ring
{
	struct bip_buffer rx_buffer;
	struct bip_buffer tx_buffer;

	struct io_interface host;
	struct io_interface device;
	io_interface_callback_t host_callback;
	io_interface_callback_t device_callback;
	void *host_context;
	void *device_context;

	void *data;
	bool allocated;
};

int io_ring_ini(struct io_ring *io_ring, void *data, size_t size);
void io_ring_fini(struct io_ring *io_ring);

struct io_ring *io_ring_create(size_t size);
void io_ring_destroy(struct io_ring *io_ring);

static inline struct io_interface *io_ring_get_device(struct io_ring *io_ring)
{
	assert(io_ring != 0);
	return &io_ring->device;
}

static inline struct io_interface *io_ring_get_host(struct io_ring *io_ring)
{
	assert(io_ring != 0);
	return &io_ring->host;
}

static inline io_interface_callback_t io_ring_get_callback(struct io_interface *interface)
{
	assert(interface != 0);
	if (interface == &interface->parent->device)
		return interface->parent->device_callback;
	else
		return interface->parent->host_callback;
}

static inline void *io_ring_get_context(struct io_interface *interface)
{
	assert(interface != 0);
	assert(interface != 0);
	if (interface == &interface->parent->device)
		return interface->parent->device_context;
	else
		return interface->parent->host_context;
}

static inline void io_ring_set_callback(struct io_interface *interface, io_interface_callback_t callback, void *context)
{
	assert(interface != 0);
	if (interface == &interface->parent->device) {
		interface->parent->device_callback = callback;
		interface->parent->device_context = context;
	} else {
		interface->parent->host_callback = callback;
		interface->parent->host_context = context;
	}
}

static inline void *io_ring_write_acquire(struct io_interface *interface, size_t *needed)
{
	assert(interface != 0);
	void *buffer = bip_buffer_write_acquire(interface->tx, needed);
	return buffer;
}

static inline void io_ring_write_release(struct io_interface *interface, size_t used)
{
	assert(interface != 0);
	bip_buffer_write_release(interface->tx, used);
	io_interface_callback_t interface_callback = *interface->callback;
	if(interface_callback != 0)
		interface_callback(interface, IO_RING_DATA_AVAIL, bip_buffer_data_available(interface->tx), *interface->context);
}

static inline void *io_ring_read_acquire(struct io_interface *interface, size_t *avail)
{
	assert(interface != 0);
	void *buffer = bip_buffer_read_acquire(interface->rx, avail);
	return buffer;
}

static inline void io_ring_read_release(struct io_interface *interface, size_t used)
{
	assert(interface != 0);
	bip_buffer_read_release(interface->rx, used);
	io_interface_callback_t interface_callback = *interface->callback;
	if(interface_callback != 0)
		interface_callback(interface, IO_RING_SPACE_AVAIL, bip_buffer_space_available(interface->rx), *interface->context);
}

static inline bool io_ring_data_available(struct io_interface *interface)
{
	assert(interface != 0);
	return !bip_buffer_is_empty(interface->rx);
}

static inline bool io_ring_space_available(struct io_interface *interface)
{
	assert(interface != 0);
	return bip_buffer_space_available(interface->tx) != 0;
}

ssize_t io_ring_read(struct io_interface *interface, void *buffer, size_t count);
ssize_t io_ring_write(struct io_interface *interface, const void *buffer, size_t count);

#endif
