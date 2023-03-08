#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <bip-buffer.h>

int bip_buffer_ini(bip_buffer_t *bip, void *data, size_t size)
{
	/* Clear the structure */
	memset(bip, 0, sizeof(bip_buffer_t));

	/* Allocate the buffer if needed */
	if (!data) {
		data = malloc(size);
		if (!data) {
			errno = ENOMEM;
			return -errno;
		}
		bip->release_buffer = true;
	}

	/* Initialize some bits */
	bip->size = size;
	bip->data = data;
	bip->allocated = false;
	bip->valid = BIP_BUFFER_VALID;

	/* All ready to go */
	return 0;
}

void bip_buffer_fini(bip_buffer_t *bip)
{
	if (bip->release_buffer)
		free(bip->data);
}

bip_buffer_t *bip_buffer_create(size_t size)
{
	assert(size > 0);

	/* Allocate a buffer with maybe with tailing data */
	bip_buffer_t *bip = malloc(sizeof(bip_buffer_t) + size);
	if (!bip) {
		errno = ENOMEM;
		return 0;
	}

	/* Initialize */
	bip_buffer_ini(bip, (void *)bip + sizeof(bip_buffer_t), size);

	/* Mark as allocated */
	bip->allocated = true;

	/* All good */
	return bip;
}

void bip_buffer_destroy(bip_buffer_t *bip)
{
	assert(bip != 0 && bip->valid == BIP_BUFFER_VALID);

	bip_buffer_fini(bip);

	/* Release the bip if allocated */
	if (bip->allocated)
		free(bip);
}

static inline void bip_buffer_switch_to_b(bip_buffer_t *bip)
{
	bip->using_b = (bip->size - bip->as) < (bip->as - bip->be);
}

void *bip_buffer_look(const bip_buffer_t *bip, size_t size)
{
	assert(bip != 0 && bip->valid == BIP_BUFFER_VALID);

	/* Is the amount of data available? */
	if (bip_buffer_is_empty(bip) || bip->size < bip->as + size)
		return 0;

	/* Return the pointer */
	return bip->data + bip->as;
}

void *bip_buffer_reserve(bip_buffer_t *bip, size_t size)
{
	void *address;

	assert(bip != 0 && bip->valid == BIP_BUFFER_VALID);

	/* Do we have room? */
	if (bip_buffer_available(bip) < size)
		return 0;

	/* Build address from either the a or b regions */
	if (bip->using_b) {
		address = bip->data + bip->be;
		bip->be += size;
	} else {
		address = bip->data + bip->ae;
		bip->ae += size;
	}

	/* Maybe switch region */
	bip_buffer_switch_to_b(bip);

	/* Return the address */
	return address;
}

void *bip_buffer_release(bip_buffer_t *bip, size_t size)
{
	void *address;

	assert(bip != 0 && bip->valid == BIP_BUFFER_VALID);

	/* Check for empty */
	if (bip_buffer_is_empty(bip) || bip->size < (bip->as + size))
		return 0;

	/* Build the address of the front */
	address = bip->data + bip->as;

	/* Move the past the requested size */
	bip->as += size;

	/* Are we now empty? */
	if (bip->as == bip->ae) {

		/* Should we replace the a region with the b region? */
		if (bip->using_b) {
			bip->as = 0;
			bip->ae = bip->be;
			bip->be = 0;
			bip->using_b = false;
		} else {
			bip->as = 0;
			bip->ae = 0;
		}
	}

	/* Maybe actually switch */
	bip_buffer_switch_to_b(bip);

	/* Return the address */
	return address;
}

ssize_t bip_buffer_push(bip_buffer_t *bip, const void *buffer, size_t count)
{
	assert(buffer != 0);

	/* Try to reserve the requested space */
	void *address = bip_buffer_reserve(bip, count);
	if (!address)
		return 0;

	/* Copy data */
	memcpy(address, buffer, count);

	/* All good */
	return count;
}

ssize_t bip_buffer_pop(bip_buffer_t *bip, void *buffer, size_t size)
{
	assert(buffer != 0);

	/* Try to release the requested space */
	void *address = bip_buffer_release(bip, size);
	if (!address)
		return 0;

	/* Copy data */
	memcpy(buffer, address, size);

	/* All good */
	return size;
}

ssize_t bip_buffer_peek(const bip_buffer_t *bip, void *buffer, size_t size)
{
	/* Try to release the requested space */
	void *address = bip_buffer_look(bip, size);
	if (!address)
		return 0;

	/* Copy data */
	memcpy(buffer, address, size);

	/* All good */
	return size;
}
