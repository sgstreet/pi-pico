#include <assert.h>

#include <cmsis/rp2040.h>

#include <init/init-sections.h>

#include <hal/rp2040/hal-rp2040.h>
#include <hal/hal-lock.h>

__fast_section void hal_lock(unsigned int lock)
{
	assert(lock < HAL_NUM_LOCKS);

	volatile uint32_t *lock_reg = &SIO->SPINLOCK0 + lock;
	while (unlikely(*lock_reg == 0));
	__DMB();
}

__fast_section bool hal_try_lock(unsigned int lock)
{
	assert(lock < HAL_NUM_LOCKS);

	volatile uint32_t *lock_reg = &SIO->SPINLOCK0 + lock;
	return *lock_reg;
}

__fast_section void hal_unlock(unsigned int lock)
{
	assert(lock < HAL_NUM_LOCKS);

	volatile uint32_t *lock_reg = &SIO->SPINLOCK0 + lock;
	*lock_reg = 0;
	__DMB();
}

static void hal_lock_init(void)
{
	for (unsigned int i = 0; i < HAL_NUM_LOCKS; i++)
		hal_unlock(i);
}
PREINIT_PLATFORM_WITH_PRIORITY(hal_lock_init, HAL_PLATFORM_PRIORITY);
