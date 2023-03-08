/*
 * Copyright (C) 2015 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ring.h
 *
 * Created on: Nov, 2015
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _RING_BUFFFER_H_
#define _RING_BUFFFER_H_

#include <stdbool.h>
#include <sys/types.h>

/* NOTE This is a power of two ring buffer and size MUST be a power of two (i.e. 2, 4, 8, 16 ....) */

struct ring_buffer
{
	unsigned int head;
	unsigned int tail;
	unsigned int size;
	char *data;
	bool allocated;
	bool release_buffer;
};

int ring_buffer_ini(struct ring_buffer *ring_buffer, void *buffer, unsigned int size);
void ring_buffer_fini(struct ring_buffer *ring_buffer);

struct ring_buffer *ring_buffer_create(unsigned int size);
void ring_buffer_destroy(struct ring_buffer *ring_buffer);

int ring_buffer_count(const struct ring_buffer *ring_buffer);
int ring_buffer_count_to_end(const struct ring_buffer *ring_buffer);
int ring_buffer_count_from(const struct ring_buffer *ring_buffer, int position);
int ring_buffer_space(const struct ring_buffer *ring_buffer);
int ring_buffer_space_to_end(const struct ring_buffer *ring_buffer);

bool ring_buffer_is_empty(const struct ring_buffer *ring_buffer);
bool ring_buffer_is_full(const struct ring_buffer *ring_buffer);

void ring_buffer_clear(struct ring_buffer *ring_buffer);

void ring_buffer_put(struct ring_buffer *ring_buffer, int value);
int ring_buffer_mput(struct ring_buffer *ring_buffer, const void *buffer, size_t size);

int ring_buffer_get(struct ring_buffer *ring_buffer);
int ring_buffer_mget(struct ring_buffer *ring_buffer, void *buffer, size_t size);

int ring_buffer_peek(struct ring_buffer *ring_buffer, int position, int *value);
int ring_buffer_mpeek(struct ring_buffer *ring_buffer, int position, void *buffer, size_t size);

static inline bool ring_buffer_space_available(const struct ring_buffer *ring_buffer)
{
	return !ring_buffer_is_full(ring_buffer);
}

static inline bool ring_buffer_not_empty(const struct ring_buffer *ring_buffer)
{
	return !ring_buffer_is_empty(ring_buffer);
}

#endif
