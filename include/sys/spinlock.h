/*
 * spinlock.h
 *
 *  Created on: Dec 26, 2022
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <stdbool.h>

void spin_lock(unsigned int spinlock);
unsigned int spin_lock_irqsave(unsigned int spinlock);

bool spin_try_lock(unsigned int spinlock);
bool spin_try_lock_irqsave(unsigned int spinlock, unsigned int *state);

void spin_unlock(unsigned int spinlock);
void spin_unlock_irqrestore(unsigned int spinlock, unsigned int state);

#endif
