#ifndef __STDATOMIC_H
#define __STDATOMIC_H

#include_next <stdatomic.h>

extern _Bool __atomic_test_and_set_m0(volatile void *mem, int model);
#undef atomic_flag_test_and_set
#define atomic_flag_test_and_set(PTR) 					\
			__atomic_test_and_set_m0 ((PTR), __ATOMIC_SEQ_CST)
#undef atomic_flag_test_and_set_explicit
#define atomic_flag_test_and_set_explicit(PTR, MO)			\
			__atomic_test_and_set_m0 ((PTR), (MO))

extern void __atomic_clear_m0 (volatile void *mem, int model);
#undef atomic_flag_clear
#define atomic_flag_clear(PTR)	__atomic_clear_m0 ((PTR), __ATOMIC_SEQ_CST)
#undef atomic_flag_clear_explicit
#define atomic_flag_clear_explicit(PTR, MO)   __atomic_clear_m0 ((PTR), (MO))

#endif
