/*
 * spinlock.c
 *
 *  Created on: Dec 26, 2022
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <sys/spinlock.h>

#include <cmsis/cmsis.h>
#include <hal/hal.h>

void spin_lock(unsigned int spinlock)
{
	hal_lock(spinlock);
}

unsigned int spin_lock_irqsave(unsigned int spinlock)
{
	unsigned int state = disable_interrupts();
	hal_lock(spinlock);
	return state;
}

bool spin_try_lock(unsigned int spinlock)
{
	return hal_try_lock(spinlock);
}

bool spin_try_lock_irqsave(unsigned int spinlock, unsigned int *state)
{
	unsigned int try_state = disable_interrupts();
	if (hal_try_lock(spinlock)) {
		*state = try_state;
		return true;
	}
	enable_interrupts(try_state);
	return false;
}

void spin_unlock(unsigned int spinlock)
{
	hal_unlock(spinlock);
}

void spin_unlock_irqrestore(unsigned int spinlock, unsigned int state)
{
	hal_unlock(spinlock);
	enable_interrupts(state);
}
