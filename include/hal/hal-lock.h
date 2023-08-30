#ifndef _HAL_LOCK_H_
#define _HAL_LOCK_H_

#include <stdint.h>
#include <stdbool.h>

void hal_lock(unsigned int lock);
bool hal_try_lock(unsigned int lock);
void hal_unlock(unsigned int lock);

#endif
