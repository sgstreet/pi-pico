/*
 * spinlock.c
 *
 *  Created on: Dec 26, 2022
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <compiler.h>

#include <sys/spinlock.h>

#include <cmsis/cmsis.h>
#include <hal/hal.h>

__fast_section void spin_lock(unsigned int spinlock)
{
	assert(spinlock < HAL_NUM_LOCKS);
	hal_lock(spinlock);
}

__fast_section unsigned int spin_lock_irqsave(unsigned int spinlock)
{
	assert(spinlock < HAL_NUM_LOCKS);
	unsigned int state = disable_interrupts();
	hal_lock(spinlock);
	return state;
}

__fast_section bool spin_try_lock(unsigned int spinlock)
{
	assert(spinlock < HAL_NUM_LOCKS);
	return hal_try_lock(spinlock);
}

__fast_section bool spin_try_lock_irqsave(unsigned int spinlock, unsigned int *state)
{
	assert(spinlock < HAL_NUM_LOCKS);
	unsigned int try_state = disable_interrupts();
	if (hal_try_lock(spinlock)) {
		*state = try_state;
		return true;
	}
	enable_interrupts(try_state);
	return false;
}

__fast_section void spin_unlock(unsigned int spinlock)
{
	assert(spinlock < HAL_NUM_LOCKS);
	hal_unlock(spinlock);
}

__fast_section void spin_unlock_irqrestore(unsigned int spinlock, unsigned int state)
{
	assert(spinlock < HAL_NUM_LOCKS);
	hal_unlock(spinlock);
	enable_interrupts(state);
}
