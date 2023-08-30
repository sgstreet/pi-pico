/*
 * sbrk.c
 *
 *  Created on: Apr 26, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <stdatomic.h>
#include <unistd.h>

extern uintptr_t __heap_start;
extern uintptr_t __heap_end;

static atomic_uintptr_t brk = (uintptr_t)&__heap_start;

void *sbrk(ptrdiff_t incr)
{
	uint32_t block = atomic_fetch_add(&brk, incr);

	if (incr < 0) {
		if (block - __heap_start < -incr) {
			atomic_fetch_sub(&brk, incr);
			return (void *)-1;
		}
	} else {
		if (__heap_end - block < incr) {
			atomic_fetch_sub(&brk, incr);
			return (void *)-1;
		}
	}

	return (void *)block;
}

