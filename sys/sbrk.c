/*
 * sbrk.c
 *
 *  Created on: Apr 26, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdint.h>
#include <stdatomic.h>
#include <unistd.h>

#include <sys/lock.h>

extern uintptr_t __heap_start;
extern uintptr_t __heap_end;

static atomic_uintptr_t brk = (uintptr_t)&__heap_start;

void *sbrk(intptr_t incr)
{
	uintptr_t block;

	/* Adjust to 8 byte alignment */
	incr = incr < 0 ? -((-incr + 7) & ~7) : ((incr + 7) & ~7);

	/* Protect the heap pointer */
	__LIBC_LOCK();

	/* Adjust break and range check */
	block = brk;
	brk += incr;
	if (brk < (uintptr_t)&__heap_start || brk > (uintptr_t)&__heap_end) {
		errno = ENOMEM;
		brk = block;
		block = -1;
	}

	/* Good or bad let the lock go */
	__LIBC_UNLOCK();

	return (void *)block;
}
