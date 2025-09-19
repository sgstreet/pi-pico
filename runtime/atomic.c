/*
 * atomic.c
 *
 *  Created on: Dec 31, 2022
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <compiler.h>
#include <config.h>

#include <cmsis/rp2040.h>

#if ATOMIC_IN_RAM == 0
#undef __fast_section
#define __fast_section
#endif

/* Must be powers of 2 */
#define ATOMIC_STRIPE 4UL
#define ATOMIC_HW_LOCKS 32UL

#define ATOMIC_HW_LOCK_INDEX 0UL
#define ATOMIC_LOCK_IDX_Pos (32 - __builtin_clz(ATOMIC_STRIPE - 1))
#define ATOMIC_LOCK_IDX_Msk (ATOMIC_HW_LOCKS - 1UL)

#define HW_LOCK_PTR(addr) (locks + ((((uintptr_t)addr) >> ATOMIC_LOCK_IDX_Pos) & ATOMIC_LOCK_IDX_Msk))

void __atomic_init(void);

static volatile uint32_t *locks = &SIO->SPINLOCK0 + ATOMIC_HW_LOCK_INDEX;

static __optimize __always_inline inline uint32_t __atomic_lock(const volatile void *mem)
{
	volatile uint32_t *const hw_lock = HW_LOCK_PTR(mem);

	uint32_t state = disable_interrupts();
	while (unlikely(*hw_lock == 0));
	__DMB();

	return state;
}

static __optimize __always_inline inline void __atomic_unlock(const volatile void *mem, uint32_t state)
{
	volatile uint32_t *const hw_lock = HW_LOCK_PTR(mem);

	__DMB();
	*hw_lock = 0;
	enable_interrupts(state);
}

__fast_section __optimize uint8_t __atomic_fetch_add_1(volatile void *mem, uint8_t val, __unused int model)
{
	volatile uint8_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint8_t result = *ptr;
	*ptr += val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint8_t __atomic_fetch_sub_1(volatile void *mem, uint8_t val, __unused int model)
{
	volatile uint8_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint8_t result = *ptr;
	*ptr -= val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint8_t __atomic_fetch_and_1(volatile void *mem, uint8_t val, __unused int model)
{
	volatile uint8_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint8_t result = *ptr;
	*ptr &= val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint8_t __atomic_fetch_or_1(volatile void *mem, uint8_t val, __unused  int model)
{
	volatile uint8_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint8_t result = *ptr;
	*ptr |= val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint8_t __atomic_exchange_1(volatile void *mem, uint8_t val, __unused int model)
{
	volatile uint8_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint8_t result = *ptr;
	*ptr = val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize bool __atomic_compare_exchange_1(volatile void *mem, void *expected, uint8_t desired, __unused bool weak, __unused int success, __unused int failure)
{
	bool result = false;
	volatile uint8_t *ptr = mem;
	uint8_t *e_ptr = expected;
	uint32_t state = __atomic_lock(mem);
	if (*ptr == *e_ptr) {
		*ptr = desired;
		result = true;
	} else
		*e_ptr = *ptr;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint16_t __atomic_fetch_add_2(volatile void *mem, uint16_t val, __unused int model)
{
	volatile uint16_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint16_t result = *ptr;
	*ptr += val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint16_t __atomic_fetch_sub_2(volatile void *mem, uint16_t val, __unused  int model)
{
	volatile uint16_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint16_t result = *ptr;
	*ptr -= val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint16_t __atomic_fetch_and_2(volatile void *mem, uint16_t val, __unused int model)
{
	volatile uint16_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint16_t result = *ptr;
	*ptr &= val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint16_t __atomic_fetch_or_2(volatile void *mem, uint16_t val, __unused int model)
{
	volatile uint16_t *ptr = mem;
	uint16_t state = __atomic_lock(mem);
	uint16_t result = *ptr;
	*ptr |= val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint16_t __atomic_exchange_2(volatile void *mem, uint16_t val, __unused int model)
{
	volatile uint16_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint16_t result = *ptr;
	*ptr = val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize bool __atomic_compare_exchange_2(volatile void *mem, void *expected, uint16_t desired, __unused bool weak, __unused int success, __unused int failure)
{
	bool result = false;
	volatile uint16_t *ptr = mem;
	uint16_t *e_ptr = expected;
	uint32_t state = __atomic_lock(mem);
	if (*ptr == *e_ptr) {
		*ptr = desired;
		result = true;
	} else
		*e_ptr = *ptr;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize unsigned int __atomic_fetch_add_4(volatile void *mem, unsigned int val, __unused int model)
{
	volatile uint32_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	unsigned int result = *ptr;
	*ptr += val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize unsigned int __atomic_fetch_sub_4(volatile void *mem, unsigned int val, __unused int model)
{
	volatile unsigned int *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	unsigned int result = *ptr;
	*ptr -= val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize unsigned int __atomic_fetch_and_4(volatile void *mem, unsigned int val, __unused int model)
{
	volatile unsigned int *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	unsigned int result = *ptr;
	*ptr &= val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize unsigned int __atomic_fetch_or_4(volatile void *mem, unsigned int val, __unused int model)
{
	volatile unsigned int *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	unsigned int result = *ptr;
	*ptr |= val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize unsigned int __atomic_exchange_4(volatile void *mem, unsigned int val, __unused int model)
{
	volatile unsigned int *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	unsigned int result = *ptr;
	*ptr = val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize bool __atomic_compare_exchange_4(volatile void *mem, void *expected, unsigned int desired, __unused bool weak, __unused int success, __unused int failure)
{
	bool result = false;
	volatile unsigned int *ptr = mem;
	unsigned int *e_ptr = expected;
	unsigned int state = __atomic_lock(mem);
	if (*ptr == *e_ptr) {
		*ptr = desired;
		result = true;
	} else
		*e_ptr = *ptr;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint64_t __atomic_fetch_add_8(volatile void *mem, uint64_t val, __unused int model)
{
	volatile uint64_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint64_t result = *ptr;
	*ptr += val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint64_t __atomic_fetch_sub_8(volatile void *mem, uint64_t val, __unused int model)
{
	volatile uint64_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint64_t result = *ptr;
	*ptr -= val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint64_t __atomic_fetch_and_8(volatile void *mem, uint64_t val, __unused int model)
{
	volatile uint64_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint64_t result = *ptr;
	*ptr &= val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint64_t __atomic_fetch_or_8(volatile void *mem, uint64_t val, __unused int model)
{
	volatile uint64_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint64_t result = *ptr;
	*ptr |= val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint64_t __atomic_exchange_8(volatile void *mem, uint64_t val, __unused int model)
{
	volatile uint64_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	uint64_t result = *ptr;
	*ptr = val;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize bool __atomic_compare_exchange_8(volatile void *mem, void *expected, uint64_t desired, __unused bool weak, __unused int success, __unused int failure)
{
	bool result = false;
	volatile uint64_t *ptr = mem;
	uint64_t *e_ptr = expected;
	uint32_t state = __atomic_lock(mem);
	if (*ptr == *e_ptr) {
		*ptr = desired;
		result = true;
	} else
		*e_ptr = *ptr;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize uint64_t __atomic_load_8(const volatile void *mem, __unused int model)
{
	const volatile uint64_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	unsigned int result = *ptr;
	__atomic_unlock(mem, state);
	return result;
}

__fast_section __optimize void __atomic_store_8(volatile void *mem, uint64_t val, __unused int model)
{
	volatile uint64_t *ptr = mem;
	uint32_t state = __atomic_lock(mem);
	*ptr = val;
	__atomic_unlock(mem, state);
}

//extern _Bool __atomic_test_and_set_m0(volatile atomic_flag *flag, int model);
//extern void __atomic_clear_m0(volatile atomic_flag *flag, int model);

//extern _Bool __atomic_test_and_set(volatile void *mem, int model);
//extern void __atomic_clear(volatile void *mem, int model);

//__fast_section __optimize bool __atomic_test_and_set_m0(volatile void *mem, int model)
//{
//	volatile bool *ptr = mem;
//	unsigned int state = __atomic_lock(mem);
//	volatile bool result = *ptr;
//	*ptr = true;
//	__atomic_unlock(mem, state);
//	return result;
//}
//
//__fast_section __optimize void __atomic_clear_m0(volatile void *mem, int model)
//{
//	volatile bool *ptr = mem;
//	*ptr = false;
//	__DMB();
//}

void __atomic_init(void)
{
	for (size_t i = ATOMIC_HW_LOCK_INDEX; i < ATOMIC_HW_LOCKS; ++i)
		locks[i] = 0;
}
