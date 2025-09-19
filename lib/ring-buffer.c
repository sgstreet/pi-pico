/*
 * Copyright (C) 2015 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ring.c
 *
 * Created on: Nov, 2015
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <ring-buffer.h>

static inline unsigned int power_two_mod(unsigned int x, unsigned int y)
{
	return x & (y - 1);
}

int ring_buffer_ini(struct ring_buffer *ring_buffer, void *buffer, unsigned int size)
{
	assert(ring_buffer != 0 && power_two_mod(size, size) == 0);

	/* Clear the structure */
	memset(ring_buffer, 0, sizeof(struct ring_buffer));

	/* Allocate the buffer if needed */
	if (!buffer) {
		buffer = malloc(size);
		if (!buffer) {
			errno = ENOMEM;
			return -1;
		}
		ring_buffer->release_buffer = true;
	}

	ring_buffer->head = 0;
	ring_buffer->tail = 0;
	ring_buffer->size = size;
	ring_buffer->data = buffer;
	ring_buffer->allocated = false;

	return 0;
}

void ring_buffer_fini(struct ring_buffer *ring_buffer)
{
	assert(ring_buffer != 0);
	if (ring_buffer->release_buffer)
		free(ring_buffer->data);
}

struct ring_buffer *ring_buffer_create(unsigned int size)
{
	assert(power_two_mod(size, size) == 0);

	struct ring_buffer *buffer = malloc(sizeof(struct ring_buffer) + size);
	if (!buffer) {
		errno = ENOMEM;
		return NULL;
	}

	ring_buffer_ini(buffer, (char *)buffer + sizeof(struct ring_buffer), size);

	buffer->allocated = true;

	return buffer;
}

void ring_buffer_destroy(struct ring_buffer *ring_buffer)
{
	assert(ring_buffer != 0);

	ring_buffer_fini(ring_buffer);

	if (ring_buffer->allocated)
		free(ring_buffer);
}

int ring_buffer_count(const struct ring_buffer *ring_buffer)
{
	assert(ring_buffer != 0);

	return (ring_buffer->head - ring_buffer->tail) & (ring_buffer->size - 1);
}

int ring_buffer_count_to_end(const struct ring_buffer *ring_buffer)
{
	assert(ring_buffer != 0);

	int end = ring_buffer->size - ring_buffer->tail;
	int count = (ring_buffer->head + end) & (ring_buffer->size - 1);

	return count < end ? count : end;
}

int ring_buffer_count_from(const struct ring_buffer *ring_buffer, int position)
{
	assert(ring_buffer != 0);

	return (ring_buffer->head - position) & (ring_buffer->size - 1);
}

int ring_buffer_space(const struct ring_buffer *ring_buffer)
{
	assert(ring_buffer != 0);

	return (ring_buffer->tail - (ring_buffer->head + 1)) & (ring_buffer->size - 1);
}

int ring_buffer_space_to_end(const struct ring_buffer *ring_buffer)
{
	assert(ring_buffer != 0);

	int end = ring_buffer->size - 1 - ring_buffer->head;
	int count = (end + ring_buffer->tail) & (ring_buffer->size - 1);

	return count < end ? count : end + 1;
}

bool ring_buffer_is_empty(const struct ring_buffer *ring_buffer)
{
	assert(ring_buffer != 0);

	return ring_buffer->head == ring_buffer->tail;
}

bool ring_buffer_is_full(const struct ring_buffer *ring_buffer)
{
	assert(ring_buffer != 0);

	return power_two_mod(ring_buffer->head + 1, ring_buffer->size) == ring_buffer->tail;
}

void ring_buffer_clear(struct ring_buffer *ring_buffer)
{
	assert(ring_buffer != 0);

	ring_buffer->head = 0;
	ring_buffer->tail = 0;
}

void ring_buffer_put(struct ring_buffer *ring_buffer, int value)
{
	assert(ring_buffer != 0);

	ring_buffer->data[ring_buffer->head] = value;
	ring_buffer->head = power_two_mod(ring_buffer->head + 1, ring_buffer->size);
	if (ring_buffer->head == ring_buffer->tail)
		ring_buffer->tail = power_two_mod(ring_buffer->tail + 1, ring_buffer->size);
}

int ring_buffer_mput(struct ring_buffer *ring_buffer, const void *buffer, size_t size)
{
	size_t amount = 0;
	const char *data = buffer;

	assert(ring_buffer != 0 && data != 0);

	while (amount < size && !ring_buffer_is_full(ring_buffer)) {
		ring_buffer->data[ring_buffer->head] = data[amount++];
		ring_buffer->head = power_two_mod(ring_buffer->head + 1, ring_buffer->size);
	}

	return amount;
}

int ring_buffer_get(struct ring_buffer *ring_buffer)
{
	assert(ring_buffer != 0);

	char value = ring_buffer->data[ring_buffer->tail];
	ring_buffer->tail = power_two_mod(ring_buffer->tail + 1, ring_buffer->size);
	return value;
}

int ring_buffer_mget(struct ring_buffer *ring_buffer, void *buffer, size_t size)
{
	size_t amount = 0;
	char *data = buffer;

	assert(ring_buffer != 0);

	/* TODO We support removing data without copy, optimize to move 'if' outside the loop */
	while (amount < size && !ring_buffer_is_empty(ring_buffer)) {
		if (data)
			data[amount] = ring_buffer->data[ring_buffer->tail];
		++amount;
		ring_buffer->tail = power_two_mod(ring_buffer->tail + 1, ring_buffer->size);
	}

	return amount;
}

int ring_buffer_peek(struct ring_buffer *ring_buffer, int position, int *value)
{
	unsigned int current = position;

	assert(ring_buffer != 0);

	/* If no current position, then initialize */
	if (position < 0)
		current = ring_buffer->tail;

	/* Return -1 of we are a the tail */
	if (current == ring_buffer->head)
		return -1;

	/* Extract the peek data from current position */
	*value = ring_buffer->data[position];

	/* Return the next position */
	return power_two_mod(position + 1, ring_buffer->size);
}

int ring_buffer_mpeek(struct ring_buffer *ring_buffer, int position, void *buffer, size_t size)
{
	size_t amount = 0;
	char *data = buffer;
	unsigned int current = position;

	assert(ring_buffer != 0);

	/* If no current position, then initialize */
	if (position < 0)
		current = ring_buffer->tail;

	/* Return -1 of we are at the end */
	if (current == ring_buffer->head)
		return -1;

	/* Extract the peek data from current position */
	while (amount < size && ring_buffer->head != current) {
		if (data)
			data[amount] = ring_buffer->data[position];
		++amount;
		position = power_two_mod(position + 1, ring_buffer->size);
	}

	/* Return the next position */
	return position;
}

