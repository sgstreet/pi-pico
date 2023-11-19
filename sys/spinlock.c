/*
 * spinlock.c
 *
 *  Created on: Dec 26, 2022
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <compiler.h>
#include <stdint.h>

#include <cmsis/cmsis.h>

#include <sys/spinlock.h>

__fast_section __optimize void spin_lock(spinlock_t *spinlock)
{
	assert(spinlock != 0);

	uint16_t ticket = atomic_fetch_add(spinlock, 1UL << 16) >> 16;
	while ((*spinlock & 0xffff) != ticket);
}

__fast_section unsigned int spin_lock_irqsave(spinlock_t *spinlock)
{
	assert(spinlock != 0);

	uint32_t state = disable_interrupts();
	spin_lock(spinlock);
	return state;
}

__fast_section __optimize bool spin_try_lock(spinlock_t *spinlock)
{
	assert(spinlock != 0);

	uint32_t value = *spinlock;
	if ((value >> 16) != (value & 0xffff))
		return false;

	return atomic_compare_exchange_strong(spinlock, &value, value + (1UL << 16));
}

__fast_section __optimize bool spin_try_lock_irqsave(spinlock_t *spinlock, unsigned int *state)
{
	assert(spinlock != 0 && state != 0);

	uint32_t irq_state = disable_interrupts();
	bool locked = spin_try_lock(spinlock);
	if (!locked) {
		enable_interrupts(irq_state);
		return false;
	}

	*state = irq_state;
	return true;
}

__fast_section __optimize void spin_unlock(spinlock_t *spinlock)
{
	assert(spinlock != 0);

	atomic_fetch_add((uint16_t *)spinlock, 1);
}

__fast_section __optimize void spin_unlock_irqrestore(spinlock_t *spinlock, unsigned int state)
{
	assert(spinlock != 0);

	spin_unlock(spinlock);
	enable_interrupts(state);
}
