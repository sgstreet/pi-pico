/*
 * spsc-bip-buffer.h
 *
 *  Created on: Feb 5, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _SPSC_BIP_BUFFER_H_
#define _SPSC_BIP_BUFFER_H_

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

struct spsc_bip_buffer
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

int spsc_bip_buffer_ini(struct spsc_bip_buffer *spsc_bip_buffer, void *data, size_t size);
void spsc_bip_buffer_fini(struct spsc_bip_buffer *spsc_bip_buffer);

struct spsc_bip_buffer *spsc_bip_buffer_create(size_t size);
void spsc_bip_buffer_destroy(struct spsc_bip_buffer *spsc_bip_buffer);

void *spsc_bip_buffer_write_acquire(struct spsc_bip_buffer *spsc_bip_buffer, size_t needed);
void spsc_bip_buffer_write_release(struct spsc_bip_buffer *spsc_bip_buffer, size_t used);

void *spsc_bip_buffer_read_acquire(struct spsc_bip_buffer *spsc_bip_buffer, size_t *avail);
void spsc_bip_buffer_read_release(struct spsc_bip_buffer *spsc_bip_buffer, size_t used);

bool spsc_bip_buffer_is_empty(struct spsc_bip_buffer *spsc_bip_buffer);
size_t spsc_bip_buffer_space_available(struct spsc_bip_buffer *spsc_bip_buffer);

#endif
