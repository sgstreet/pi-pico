/*
 * spinlock.h
 *
 *  Created on: Dec 26, 2022
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <stdatomic.h>
#include <stdbool.h>

typedef atomic_ulong spinlock_t;

void spin_lock(spinlock_t *spinlock);
unsigned int spin_lock_irqsave(spinlock_t *spinlock);

bool spin_try_lock(spinlock_t *spinlock);
bool spin_try_lock_irqsave(spinlock_t *spinlock, unsigned int *state);

void spin_unlock(spinlock_t *spinlock);
void spin_unlock_irqrestore(spinlock_t *spinlock, unsigned int state);

#endif
