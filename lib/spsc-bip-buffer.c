/*
 * spsc-bip-buffer.c
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

#include <spsc-bip-buffer.h>

int spsc_bip_buffer_ini(struct spsc_bip_buffer *spsc_bip_buffer, void *data, size_t size)
{
	assert(spsc_bip_buffer != 0);

	/* Always clear the buffer */
	memset(spsc_bip_buffer, 0, sizeof(struct spsc_bip_buffer));

	/* Allocated space for the buffer, if needed */
	if (!data) {
		data = calloc(size, 1);
		if (!data)
			return -errno;
		spsc_bip_buffer->allocated = true;
	}

	/* Initialize it */
	spsc_bip_buffer->data = data;
	spsc_bip_buffer->size = size;

	/* All good */
	return 0;
}

void spsc_bip_buffer_fini(struct spsc_bip_buffer *spsc_bip_buffer)
{
	assert(spsc_bip_buffer != 0);

	/* If we are managing the buffer, release it */
	if (spsc_bip_buffer->allocated)
		free(spsc_bip_buffer->data);
}

struct spsc_bip_buffer *spsc_bip_buffer_create(size_t size)
{
	/* Allocate the buffer */
	struct spsc_bip_buffer *spsc_bip_buffer = malloc(sizeof(struct spsc_bip_buffer) + size);
	if (!spsc_bip_buffer)
		return 0;

	/* Forward to initializer */
	int status = spsc_bip_buffer_ini(spsc_bip_buffer, spsc_bip_buffer + sizeof(struct spsc_bip_buffer), size);
	if (status != 0) {
		free(spsc_bip_buffer);
		return 0;
	}

	/* Looks good */
	return spsc_bip_buffer;
}

void spsc_bip_buffer_destroy(struct spsc_bip_buffer *spsc_bip_buffer)
{
	assert(spsc_bip_buffer != 0);

	/* Forward to the finalizer */
	spsc_bip_buffer_fini(spsc_bip_buffer);

	/* Release our memory */
	free(spsc_bip_buffer);
}

void *spsc_bip_buffer_write_acquire(struct spsc_bip_buffer *spsc_bip_buffer, size_t needed)
{
	assert(spsc_bip_buffer != 0);

	/* Load stable data set */
	const size_t write_index = spsc_bip_buffer->write_index;
	const size_t read_index = spsc_bip_buffer->read_index;
	const size_t linear = spsc_bip_buffer->size - read_index;
	const size_t avail = read_index > write_index ?  read_index - write_index - 1 : spsc_bip_buffer->size - (write_index - read_index) - 1;
	const size_t linear_avail = avail < linear ? avail : linear;

	/* Room at the end of the buffer? */
	if (needed <= linear_avail)
		return spsc_bip_buffer->data + write_index;

	/* How about at the beginning? */
	if (needed <= avail - linear_avail) {
		spsc_bip_buffer->write_wrapped = true;
		return spsc_bip_buffer->data;
	}

	/* Nothing available */
	return 0;
}

void spsc_bip_buffer_write_release(struct spsc_bip_buffer *spsc_bip_buffer, size_t used)
{
	assert(spsc_bip_buffer != 0);

	size_t write_index = spsc_bip_buffer->write_index;
	size_t invalidate_index = spsc_bip_buffer->invalidate_index;

	/* Update the invalidate index if the write acquire wrapped the buffer */
	if (spsc_bip_buffer->write_wrapped) {
		spsc_bip_buffer->write_wrapped = false;
		invalidate_index = write_index;
		write_index = 0;
	}

	/* Update the write index, wrapping if needed */
	write_index += used;
	if (write_index == spsc_bip_buffer->size)
		write_index = 0;

	/* Did write we write over the invalidated parts? */
	if (write_index > invalidate_index)
		invalidate_index = write_index;

	/* Done update the indexes */
	spsc_bip_buffer->invalidate_index = invalidate_index;
	spsc_bip_buffer->write_index = write_index;
}

void *spsc_bip_buffer_read_acquire(struct spsc_bip_buffer *spsc_bip_buffer, size_t *avail)
{
	assert(spsc_bip_buffer != 0 && avail != 0);

	/* Load the indexes */
	const size_t write_index = spsc_bip_buffer->write_index;
	const size_t invalid_index = spsc_bip_buffer->invalidate_index;
	const size_t read_index = spsc_bip_buffer->read_index;

	/* Empty? */
	if (read_index == write_index) {
		*avail = 0;
		return 0;
	}

	/* If read index is behind the write index */
	if (read_index < write_index) {
		*avail = write_index - read_index;
		return spsc_bip_buffer->data + read_index;
	}

	/* Has the read index reached the invalidated index, wrapped the read */
	if (read_index == invalid_index) {
		spsc_bip_buffer->read_wrapped = true;
		*avail = write_index;
		return spsc_bip_buffer->data;
	}

	/* Data available between the invalidate index and the read index */
	*avail = invalid_index - read_index;
	return spsc_bip_buffer->data + read_index;
}

void spsc_bip_buffer_read_release(struct spsc_bip_buffer *spsc_bip_buffer, size_t used)
{
	assert(spsc_bip_buffer != 0);

	/* Load the read index */
	size_t read_index = spsc_bip_buffer->read_index;

	/* Did the read wrap? if so reset the read index */
	if (spsc_bip_buffer->read_wrapped) {
		spsc_bip_buffer->read_wrapped = false;
		read_index = 0;
	}

	/* Update the read index, wrapping if needed */
	read_index += used;
	if (read_index == spsc_bip_buffer->size)
		read_index = 0;

	/* Update the read index */
	spsc_bip_buffer->read_index = read_index;
}

bool spsc_bip_buffer_is_empty(struct spsc_bip_buffer *spsc_bip_buffer)
{
	return spsc_bip_buffer->read_index == spsc_bip_buffer->write_index;
}

size_t spsc_bip_buffer_space_available(struct spsc_bip_buffer *spsc_bip_buffer)
{
	assert(spsc_bip_buffer != 0);

	/* Load stable data set */
	const size_t write_index = spsc_bip_buffer->write_index;
	const size_t read_index = spsc_bip_buffer->read_index;
	const size_t linear = spsc_bip_buffer->size - read_index;
	const size_t avail = read_index > write_index ?  read_index - write_index - 1 : spsc_bip_buffer->size - (write_index - read_index) - 1;

	/* Return the amount a linear space available */
	return avail < linear ? avail : linear;
}
