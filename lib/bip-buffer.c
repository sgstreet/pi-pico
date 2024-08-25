/*
 * bip-buffer.c
 *
 *  Created on: Feb 5, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * Copyright (c) 2022 Djordje Nedic
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall
 * be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * This is based on part of LFBB - Lock Free Bipartite Buffer
 * Author: Djordje Nedic <nedic.djordje2@gmail.com>
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <bip-buffer.h>

int bip_buffer_ini(struct bip_buffer *bip_buffer, void *data, size_t size)
{
	assert(bip_buffer != 0);

	/* Always clear the buffer */
	memset(bip_buffer, 0, sizeof(struct bip_buffer));

	/* Allocated space for the buffer, if needed */
	if (!data) {
		data = calloc(size, 1);
		if (!data)
			return -errno;
		bip_buffer->allocated = true;
	}

	/* Initialize it */
	bip_buffer->data = data;
	bip_buffer->size = size;

	/* All good */
	return 0;
}

void bip_buffer_fini(struct bip_buffer *bip_buffer)
{
	assert(bip_buffer != 0);

	/* If we are managing the buffer, release it */
	if (bip_buffer->allocated)
		free(bip_buffer->data);
}

struct bip_buffer *bip_buffer_create(size_t size)
{
	/* Allocate the buffer */
	struct bip_buffer *bip_buffer = malloc(sizeof(struct bip_buffer) + size);
	if (!bip_buffer)
		return 0;

	/* Forward to initializer */
	int status = bip_buffer_ini(bip_buffer, bip_buffer + sizeof(struct bip_buffer), size);
	if (status != 0) {
		free(bip_buffer);
		return 0;
	}

	/* Looks good */
	return bip_buffer;
}

void bip_buffer_destroy(struct bip_buffer *bip_buffer)
{
	assert(bip_buffer != 0);

	/* Forward to the finalizer */
	bip_buffer_fini(bip_buffer);

	/* Release our memory */
	free(bip_buffer);
}

void *bip_buffer_write_acquire(struct bip_buffer *bip_buffer, size_t *needed)
{
	assert(bip_buffer != 0 && needed != 0);

	/* Load stable data set */
	const size_t write_index = atomic_load_explicit(&bip_buffer->write_index, memory_order_relaxed);
	const size_t read_index = atomic_load_explicit(&bip_buffer->read_index, memory_order_acquire);

	const size_t linear = bip_buffer->size - write_index;
	const size_t avail = read_index > write_index ?  read_index - write_index - 1 : bip_buffer->size - (write_index - read_index) - 1;
	const size_t linear_avail = avail < linear ? avail : linear;

	/* Get the max available if requested */
	if (*needed == SIZE_MAX)
		*needed = linear_avail < avail - linear_avail ? avail - linear_avail : linear_avail;

	/* Check to see if there was any space available */
	if (*needed > 0) {

		/* Room at the end of the buffer? */
		if (*needed <= linear_avail)
			return bip_buffer->data + write_index;

		/* How about at the beginning? */
		if (*needed <= avail - linear_avail) {
			bip_buffer->write_wrapped = true;
			return bip_buffer->data;
		}
	}

	/* Nothing available */
	return 0;
}

void bip_buffer_write_release(struct bip_buffer *bip_buffer, size_t used)
{
	assert(bip_buffer != 0);

	size_t write_index = atomic_load_explicit(&bip_buffer->write_index, memory_order_relaxed);

	/* If the write wrapped set the invalidate index and reset write index*/
	size_t invalidate_index;
	if (bip_buffer->write_wrapped) {
		bip_buffer->write_wrapped = false;
		invalidate_index = write_index;
		write_index = 0U;
	} else
		invalidate_index = atomic_load_explicit(&bip_buffer->invalidate_index, memory_order_relaxed);

	/* Update the write index */
	write_index += used;

	/* Did write we write over the invalidated parts? */
	if (write_index > invalidate_index)
		invalidate_index = write_index;

	/* Update the write index, wrapping if needed */
	if (write_index == bip_buffer->size)
		write_index = 0;

	/* Done update the indexes */
	atomic_store_explicit(&bip_buffer->invalidate_index, invalidate_index, memory_order_relaxed);
	atomic_store_explicit(&bip_buffer->write_index, write_index, memory_order_release);
}

void *bip_buffer_read_acquire(struct bip_buffer *bip_buffer, size_t *avail)
{
	assert(bip_buffer != 0 && avail != 0);

	/* Load the indexes */
	const size_t read_index = atomic_load_explicit(&bip_buffer->read_index, memory_order_relaxed);
	const size_t write_index = atomic_load_explicit(&bip_buffer->write_index, memory_order_acquire);

	/* Empty? */
	if (read_index == write_index) {
		*avail = 0;
		return 0;
	}

	/* If read index is behind the write index */
	if (read_index < write_index) {
		*avail = write_index - read_index;
		return bip_buffer->data + read_index;
	}

	/* Has the read index reached the invalidated index, wrapped the read */
	const size_t invalid_index = atomic_load_explicit(&bip_buffer->invalidate_index, memory_order_relaxed);
	if (read_index == invalid_index) {
		bip_buffer->read_wrapped = true;
		*avail = write_index;
		return bip_buffer->data;
	}

	/* Data available between the invalidate index and the read index */
	*avail = invalid_index - read_index;
	return bip_buffer->data + read_index;
}

void bip_buffer_read_release(struct bip_buffer *bip_buffer, size_t used)
{
	assert(bip_buffer != 0);

	/* If the read wrapped, overflow the read index */
	size_t read_index;
	if (bip_buffer->read_wrapped) {
		bip_buffer->read_wrapped = false;
		read_index = 0U;
	} else
		read_index = atomic_load_explicit(&bip_buffer->read_index, memory_order_relaxed);

	/* Increment the read index and wrap to 0 if needed */
	read_index += used;
	if (read_index == bip_buffer->size)
		read_index = 0U;

	/* Store the indexes with adequate memory ordering */
	atomic_store_explicit(&bip_buffer->read_index, read_index, memory_order_release);
}

bool bip_buffer_is_empty(struct bip_buffer *bip_buffer)
{
	return bip_buffer->read_index == bip_buffer->write_index;
}

size_t bip_buffer_space_available(struct bip_buffer *bip_buffer)
{
	assert(bip_buffer != 0);

	/* Load stable data set */
	const size_t write_index = bip_buffer->write_index;
	const size_t read_index = bip_buffer->read_index;
	const size_t avail = read_index > write_index ?  read_index - write_index - 1 : bip_buffer->size - (write_index - read_index) - 1;
	return avail;
}


size_t bip_buffer_data_available(struct bip_buffer *bip_buffer)
{
	assert(bip_buffer != 0);
	return bip_buffer->size - bip_buffer_space_available(bip_buffer) - 1;
}
