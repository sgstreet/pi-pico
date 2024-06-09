/*
 * io-ring.c
 *
 *  Created on: Dec 2, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <io-ring.h>

int io_ring_ini(struct io_ring *io_ring, void *data, size_t size)
{
	int status;

	assert(io_ring != 0);

	/* Basic initialization */
	memset(io_ring, 0, sizeof(struct io_ring));

	/* Do we need to allocate the buffer memory? */
	if (data == 0) {
		io_ring->data = malloc(size);
		if (!io_ring->data)
			return -errno;
		io_ring->allocated = true;
	}

	/* Initialize the rx buffer */
	status = bip_buffer_ini(&io_ring->rx_buffer, io_ring->data, size >> 1);
	if (status < 0)
		goto release_data;

	/* Now the tx buffer */
	status = bip_buffer_ini(&io_ring->tx_buffer, io_ring->data + (size >> 1), size >> 1);
	if (status < 0)
		goto release_rx_buffer;

	/* Setup the interfaces */
	io_ring->host.rx = &io_ring->rx_buffer;
	io_ring->host.tx = &io_ring->tx_buffer;
	io_ring->host.callback = &io_ring->device_callback;
	io_ring->host.context = &io_ring->device_context;
	io_ring->host.parent = io_ring;

	io_ring->device.rx = &io_ring->tx_buffer;
	io_ring->device.tx = &io_ring->rx_buffer;
	io_ring->device.callback = &io_ring->host_callback;
	io_ring->device.context = &io_ring->host_context;
	io_ring->device.parent = io_ring;

	/* All good */
	return 0;

release_rx_buffer:
	bip_buffer_fini(&io_ring->rx_buffer);

release_data:
	if (io_ring->allocated)
		free(data);

	return status;
}

void io_ring_fini(struct io_ring *io_ring)
{
	assert(io_ring != 0);

	/* First clean up the buffers */
	bip_buffer_fini(&io_ring->tx_buffer);
	bip_buffer_fini(&io_ring->rx_buffer);

	/* Now any allocated memory */
	if (io_ring->allocated)
		free(io_ring->data);
}

struct io_ring *io_ring_create(size_t size)
{
	/* Allocate the buffer */
	struct io_ring *io_ring = malloc(sizeof(struct io_ring) + size);
	if (!io_ring)
		return 0;

	/* Forward to initializer */
	int status = io_ring_ini(io_ring, io_ring + sizeof(struct io_ring), size);
	if (status != 0) {
		free(io_ring);
		return 0;
	}

	/* Looks good */
	return io_ring;
}

void io_ring_destroy(struct io_ring *io_ring)
{
	assert(io_ring != 0);

	/* Clean up the io ring */
	io_ring_fini(io_ring);

	/* Now our memory */
	free(io_ring);
}

ssize_t io_ring_read(struct io_interface *interface, void *buffer, size_t count)
{
	assert(interface != 0 && buffer != 0);

	/* Data available in the io ring? */
	size_t amount = 0;
	void *data = io_ring_read_acquire(interface, &amount);
	if (amount > 0) {
		amount = amount < count ? amount : count;
		memcpy(buffer, data, amount);
		io_ring_read_release(interface, amount);
	}

	/* Return amount read which may be less than count or even zero */
	return amount;
}

ssize_t io_ring_write(struct io_interface *interface, const void *buffer, size_t count)
{
	assert(interface != 0 && buffer != 0);

	/* Get free space in the tx buffer and copy the buffer into it*/
	size_t amount = SIZE_MAX;
	void *data = io_ring_write_acquire(interface, &amount);
	if (amount > 0) {
		amount = amount < count ? amount : count;
		memcpy(data, buffer, amount);
		io_ring_write_release(interface, amount);
	}

	/* Return amount written which may be less than count or even zero */
	return amount;
}

