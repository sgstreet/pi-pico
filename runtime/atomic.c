/*
 * atomic.c
 *
 *  Created on: Dec 31, 2022
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <compiler.h>
#include <config.h>

#if ATOMIC_IN_RAM == 0
#undef __fast_section
#define __fast_section
#endif

void __atomic_init(void);

static __optimize __always_inline inline uint32_t __atomic_lock(void)
{
	__unused uint32_t locks = SIO->SPINLOCK_ST;
	uint32_t state = disable_interrupts();
	while (unlikely(SIO->SPINLOCK0 == 0));
	__DMB();
	return state;
}

static __optimize __always_inline inline void __atomic_unlock(uint32_t state)
{
	SIO->SPINLOCK0 = 0;
	__DMB();
	enable_interrupts(state);
}

__fast_section __optimize uint32_t __atomic_fetch_add_4(volatile void *mem, uint32_t val, int model)
{
	volatile uint32_t *ptr = mem;
	uint32_t state = __atomic_lock();
	uint32_t result = *ptr;
	*ptr += val;
	__atomic_unlock(state);
	return result;
}

__fast_section __optimize uint32_t __atomic_fetch_sub_4(volatile void *mem, uint32_t val, int model)
{
	volatile uint32_t *ptr = mem;
	uint32_t state = __atomic_lock();
	uint32_t result = *ptr;
	*ptr -= val;
	__atomic_unlock(state);
	return result;
}

__fast_section __optimize uint32_t __atomic_fetch_and_4(volatile void *mem, uint32_t val, int model)
{
	volatile uint32_t *ptr = mem;
	uint32_t state = __atomic_lock();
	uint32_t result = *ptr;
	*ptr &= val;
	__atomic_unlock(state);
	return result;
}

__fast_section __optimize uint32_t __atomic_fetch_or_4(volatile void *mem, uint32_t val, int model)
{
	volatile uint32_t *ptr = mem;
	uint32_t state = __atomic_lock();
	uint32_t result = *ptr;
	*ptr |= val;
	__atomic_unlock(state);
	return result;
}

__fast_section __optimize uint32_t __atomic_exchange_4(volatile void *mem, uint32_t val, int model)
{
	volatile uint32_t *ptr = mem;
	uint32_t state = __atomic_lock();
	uint32_t result = *ptr;
	*ptr = val;
	__atomic_unlock(state);
	return result;
}

__fast_section __optimize bool __atomic_compare_exchange_4(volatile void *mem, void *expected, uint32_t desired, bool weak, int success, int failure)
{
	bool result = false;
	volatile uint32_t *ptr = mem;
	uint32_t *e_ptr = expected;
	uint32_t state = __atomic_lock();
	if (*ptr == *e_ptr) {
		*ptr = desired;
		result = true;
	} else
		*e_ptr = *ptr;
	__atomic_unlock(state);
	return result;
}

__fast_section __optimize bool __atomic_test_and_set_m0(volatile void *mem, int model)
{
	volatile bool *ptr = mem;
	uint32_t state = __atomic_lock();
	volatile bool result = *ptr;
	*ptr = true;
	__atomic_unlock(state);
	return result;
}

__fast_section __optimize void __atomic_clear_m0(volatile void *mem, int model)
{
	volatile bool *ptr = mem;
	/* TODO Is as spin_lock good enough here, I think so? */
	uint32_t state = __atomic_lock();
	*ptr = false;
	__atomic_unlock(state);
}

__fast_section __optimize uint64_t __atomic_load_8(volatile void *mem, int model)
{
	volatile uint64_t *ptr = mem;
	uint32_t state = __atomic_lock();
	uint32_t result = *ptr;
	__atomic_unlock(state);
	return result;
}

__fast_section __optimize void __atomic_store_8(volatile void *mem, uint64_t val, int model)
{
	volatile uint64_t *ptr = mem;
	uint32_t state = __atomic_lock();
	*ptr = val;
	__atomic_unlock(state);
}

void __atomic_init(void)
{
	SIO->SPINLOCK0 = 0;
}
