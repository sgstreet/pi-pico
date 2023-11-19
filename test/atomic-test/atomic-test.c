#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdatomic.h>
#include <compiler.h>

#include <cmsis/cmsis.h>

#define ATOMIC_STRIPE 4
#define ATOMIC_HW_LOCKS 32
#define ATOMIC_HW_LOCK_INDEX 0
#define ATOMIC_LOCK_IDX_Pos (ATOMIC_STRIPE / 2)
#define ATOMIC_LOCK_IDX_Msk (ATOMIC_HW_LOCKS - 1)

#define LOCK_IDX(addr) ((((uintptr_t)addr) >> ATOMIC_LOCK_IDX_Pos) & ATOMIC_LOCK_IDX_Msk)

typedef struct spin_lock
{
	uint32_t value;
} spin_lock_t;

static void spin_lock_init(spin_lock_t *lock);
static void spin_lock(spin_lock_t *lock);
static void spin_unlock(spin_lock_t *lock);

static void spin_lock_init(spin_lock_t *lock)
{
	lock->value = 0;
}

static void spin_lock(spin_lock_t *lock)
{
	uint32_t value = atomic_fetch_add(&lock->value, 1UL << 16);
	uint16_t ticket = value >> 16;

	if (ticket == (uint16_t)value)
		return;

	while (atomic_load(&value) != ticket)
		__WFE();
}

static void spin_unlock(spin_lock_t *lock)
{
	uint16_t *ptr = (uint16_t *)lock;
	uint32_t value = atomic_load(&lock->value);

	atomic_store(ptr, (uint16_t)value + 1);

	__DSB();
	__SEV();
}

static bool spin_try_lock(spin_lock_t *lock)
{
	uint32_t value = atomic_load(&lock->value);

	if ((value >> 16) != (value & 0xffff))
		return false;

	return atomic_compare_exchange_strong(&lock->value, &value, value + (1UL << 16));
}

static bool spin_is_locked(spin_lock_t *lock)
{
	uint32_t value = atomic_load(&lock->value);
	return ((value >> 16) != (value & 0xffff));
}

//static bool spin_is_contended(spin_lock_t *lock)
//{
//	uint32_t value = atomic_load(&lock->value);
//	return (int16_t)((value >> 16) - (value & 0xffff)) > 1;
//}

int main(int argc, char **argv)
{
	spin_lock_t locks[16];

	for (size_t i = 0; i < array_sizeof(locks); ++i)
		spin_lock_init(&locks[i]);

	spin_lock(&locks[0]);
	spin_unlock(&locks[0]);
	__unused bool locked = spin_try_lock(&locks[0]);
	locked = spin_is_locked(&locks[0]);
	locked = spin_try_lock(&locks[0]);
	spin_unlock(&locks[0]);

	for (size_t i = 0; i < array_sizeof(locks); ++i)
		spin_lock(&locks[i]);

	for (size_t i = 0; i < array_sizeof(locks); ++i)
		spin_unlock(&locks[i]);

}

