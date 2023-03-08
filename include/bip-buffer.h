/*
 * bip-buffer.h
 *
 *  Created on: Nov 13, 2015
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef BIP_BUFFER_H_
#define BIP_BUFFER_H_

#include <assert.h>
#include <stdbool.h>

#include <sys/types.h>

#define BIP_BUFFER_VALID 0x43eb6d43UL

typedef struct bip_buffer
{
	unsigned long int valid;
	bool allocated;
	bool release_buffer;
	unsigned long int size;
	unsigned long int as;
	unsigned long int ae;
	unsigned long int be;
	bool using_b;
	void *data;
} bip_buffer_t;

int bip_buffer_ini(bip_buffer_t *bip, void *data, size_t size);
void bip_buffer_fini(bip_buffer_t *bip);

bip_buffer_t *bip_buffer_create(size_t size);
void bip_buffer_destroy(bip_buffer_t *bip);

void *bip_buffer_look(const bip_buffer_t *bip, size_t size);
void *bip_buffer_reserve(bip_buffer_t *bip, size_t size);
void *bip_buffer_release(bip_buffer_t *bip, size_t size);

ssize_t bip_buffer_push(bip_buffer_t *bip, const void *buffer, size_t count);
ssize_t bip_buffer_pop(bip_buffer_t *bip, void *buffer, size_t size);
ssize_t bip_buffer_peek(const bip_buffer_t *bip, void *buffer, size_t size);

static inline bool bip_buffer_is_empty(const bip_buffer_t *bip)
{
	assert(bip != 0 && bip->valid == BIP_BUFFER_VALID);
	return bip->as == bip->ae;
}

static inline size_t bip_buffer_available(const bip_buffer_t *bip)
{
	assert(bip != 0 && bip->valid == BIP_BUFFER_VALID);
	return bip->using_b ? bip->as - bip->be : bip->size - bip->ae;
}

static inline size_t bip_buffer_used(const bip_buffer_t *bip)
{
	assert(bip != 0 && bip->valid == BIP_BUFFER_VALID);
	return (bip->ae - bip->as) + bip->be;
}

static inline size_t bip_buffer_size(const bip_buffer_t *bip)
{
	assert(bip != 0 && bip->valid == BIP_BUFFER_VALID);
	return bip->size;
}

#endif

