/*
 * atomic.c
 *
 *  Created on: Dec 31, 2022
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <config.h>

#include <sys/spinlock.h>

//uint32_t __atomic_load_4(volatile void *mem, int model)
//{
//	volatile uint32_t *ptr = mem;
//	/* TODO Is as spin_lock good enough here, I think so? */
//	uint32_t state = spin_lock_irqsave(ATOMIC_HW_SPINLOCK);
//	uint32_t result = *ptr;
//	spin_unlock_irqrestore(ATOMIC_HW_SPINLOCK, state);
//	return result;
//}
//
//void __atomic_store_4(volatile void *mem, uint32_t val, int model)
//{
//	volatile uint32_t *ptr = mem;
//	/* TODO Is as spin_lock good enough here, I think so? */
//	uint32_t state = spin_lock_irqsave(ATOMIC_HW_SPINLOCK);
//	*ptr = val;
//	spin_unlock_irqrestore(ATOMIC_HW_SPINLOCK, state);
//}

__fast_section __optimize uint32_t __atomic_fetch_add_4(volatile void *mem, uint32_t val, int model)
{
	volatile uint32_t *ptr = mem;
	uint32_t state = spin_lock_irqsave(ATOMIC_HW_SPINLOCK);
	uint32_t result = *ptr;
	*ptr += val;
	spin_unlock_irqrestore(ATOMIC_HW_SPINLOCK, state);
	return result;
}

__fast_section __optimize uint32_t __atomic_fetch_sub_4(volatile void *mem, uint32_t val, int model)
{
	volatile uint32_t *ptr = mem;
	uint32_t state = spin_lock_irqsave(ATOMIC_HW_SPINLOCK);
	uint32_t result = *ptr;
	*ptr -= val;
	spin_unlock_irqrestore(ATOMIC_HW_SPINLOCK, state);
	return result;
}

__fast_section __optimize uint32_t __atomic_fetch_and_4(volatile void *mem, uint32_t val, int model)
{
	volatile uint32_t *ptr = mem;
	uint32_t state = spin_lock_irqsave(ATOMIC_HW_SPINLOCK);
	uint32_t result = *ptr;
	*ptr &= val;
	spin_unlock_irqrestore(ATOMIC_HW_SPINLOCK, state);
	return result;
}

__fast_section __optimize uint32_t __atomic_fetch_or_4(volatile void *mem, uint32_t val, int model)
{
	volatile uint32_t *ptr = mem;
	uint32_t state = spin_lock_irqsave(ATOMIC_HW_SPINLOCK);
	uint32_t result = *ptr;
	*ptr |= val;
	spin_unlock_irqrestore(ATOMIC_HW_SPINLOCK, state);
	return result;
}

__fast_section __optimize uint32_t __atomic_exchange_4(volatile void *mem, uint32_t val, int model)
{
	volatile uint32_t *ptr = mem;
	uint32_t state = spin_lock_irqsave(ATOMIC_HW_SPINLOCK);
	uint32_t result = *ptr;
	*ptr = val;
	spin_unlock_irqrestore(ATOMIC_HW_SPINLOCK, state);
	return result;
}

__fast_section __optimize bool __atomic_compare_exchange_4(volatile void *mem, void *expected, uint32_t desired, bool weak, int success, int failure)
{
	bool result = false;
	volatile uint32_t *ptr = mem;
	uint32_t *e_ptr = expected;
	uint32_t state = spin_lock_irqsave(ATOMIC_HW_SPINLOCK);
	if (*ptr == *e_ptr) {
		*ptr = desired;
		result = true;
	} else
		*e_ptr = *ptr;
	spin_unlock_irqrestore(ATOMIC_HW_SPINLOCK, state);
	return result;
}

__fast_section __optimize bool __atomic_test_and_set_m0(volatile void *mem, int model)
{
	volatile bool *ptr = mem;
	uint32_t state = spin_lock_irqsave(ATOMIC_HW_SPINLOCK);
	volatile bool result = *ptr;
	*ptr = true;
	spin_unlock_irqrestore(ATOMIC_HW_SPINLOCK, state);
	return result;
}

__fast_section __optimize void __atomic_clear_m0(volatile void *mem, int model)
{
	volatile bool *ptr = mem;
	/* TODO Is as spin_lock good enough here, I think so? */
	uint32_t state = spin_lock_irqsave(ATOMIC_HW_SPINLOCK);
	*ptr = false;
	spin_unlock_irqrestore(ATOMIC_HW_SPINLOCK, state);
}
