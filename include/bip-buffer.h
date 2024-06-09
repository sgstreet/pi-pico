/*
 * bip-buffer.h
 *
 *  Created on: Feb 5, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _BIP_BUFFER_H_
#define _BIP_BUFFER_H_

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

struct bip_buffer
{
	atomic_ulong read_index;
	atomic_ulong write_index;
	atomic_ulong invalidate_index;
	bool write_wrapped;
	bool read_wrapped;

	bool allocated;
	size_t size;
	void *data;
};

int bip_buffer_ini(struct bip_buffer *bip_buffer, void *data, size_t size);
void bip_buffer_fini(struct bip_buffer *bip_buffer);

struct bip_buffer *bip_buffer_create(size_t size);
void bip_buffer_destroy(struct bip_buffer *bip_buffer);

void *bip_buffer_write_acquire(struct bip_buffer *bip_buffer, size_t *needed);
void bip_buffer_write_release(struct bip_buffer *bip_buffer, size_t used);

void *bip_buffer_read_acquire(struct bip_buffer *bip_buffer, size_t *avail);
void bip_buffer_read_release(struct bip_buffer *bip_buffer, size_t used);

bool bip_buffer_is_empty(struct bip_buffer *bip_buffer);
size_t bip_buffer_space_available(struct bip_buffer *bip_buffer);
size_t bip_buffer_data_available(struct bip_buffer *bip_buffer);

#endif
