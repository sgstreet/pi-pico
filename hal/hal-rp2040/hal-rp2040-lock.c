#include <assert.h>
#include <string.h>

#include <cmsis/rp2040.h>

#include <init/init-sections.h>

#include <hal/rp2040/hal-rp2040.h>
#include <hal/hal-lock.h>

__fast_section void hal_lock(unsigned int lock)
{
	assert(lock < HAL_NUM_LOCKS);
//	__unused uint32_t state = SIO->SPINLOCK_ST;
	volatile uint32_t *lock_reg = &SIO->SPINLOCK0 + lock;
	while (unlikely(*lock_reg == 0))
		__WFE();
	__DMB();
}

__fast_section bool hal_try_lock(unsigned int lock)
{
	assert(lock < HAL_NUM_LOCKS);
//	__unused uint32_t state = SIO->SPINLOCK_ST;
	volatile uint32_t *lock_reg = &SIO->SPINLOCK0 + lock;
	return *lock_reg != 0;
}

__fast_section void hal_unlock(unsigned int lock)
{
	assert(lock < HAL_NUM_LOCKS);
//	__unused uint32_t state = SIO->SPINLOCK_ST;
	volatile uint32_t *lock_reg = &SIO->SPINLOCK0 + lock;
	*lock_reg = 0;
	__SEV();
	__DMB();
}

static void hal_lock_init(void)
{
	/* We skip the first lock which is assigned to the M0+ atomic infrastructure */
	for (unsigned int i = 1; i < HAL_NUM_LOCKS; ++i)
		*(&SIO->SPINLOCK0 + i) = 0;
}
PREINIT_PLATFORM_WITH_PRIORITY(hal_lock_init, HAL_PLATFORM_PRIORITY);
