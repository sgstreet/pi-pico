#ifndef __STDATOMIC_H
#define __STDATOMIC_H

#include_next <stdatomic.h>

#undef atomic_flag_test_and_set
#undef atomic_flag_test_and_set_explicit
#undef atomic_flag_clear
#undef atomic_flag_clear_explicit

#define atomic_flag_test_and_set(PTR) __atomic_test_and_set_m0((PTR), __ATOMIC_SEQ_CST)
#define atomic_flag_test_and_set_explicit(PTR, MO) __atomic_test_and_set_m0((PTR), (MO))

#define atomic_flag_clear(PTR) __atomic_clear_m0((PTR), __ATOMIC_SEQ_CST)
#define atomic_flag_clear_explicit(PTR, MO) __atomic_clear_m0((PTR), (MO))

static __attribute__((always_inline)) inline _Bool __atomic_test_and_set_m0(volatile atomic_flag *flag, int model)
{
#if __GCC_ATOMIC_TEST_AND_SET_TRUEVAL == 1
	return atomic_exchange_explicit((volatile _Bool *)flag, 1, model);
#else
	return atomic_exchange_explicit((volatile unsigned char *)flag, 1, model);
#endif
}

static __attribute__((always_inline)) inline void __atomic_clear_m0(volatile atomic_flag *flag, int model)
{
#if __GCC_ATOMIC_TEST_AND_SET_TRUEVAL == 1
	atomic_store_explicit((volatile _Bool *)flag, 0, model);
#else
	atomic_store_explicit((volatile unsigned char *)flag, 0, model);
#endif
}

#endif
