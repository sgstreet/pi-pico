#ifndef _CMSIS_H_
#define _CMSIS_H_

#include <stdint.h>

#include <compiler.h>
#include <cmsis/cmsis_compiler.h>
#include <cmsis/rp2040.h>

#define __LAST_IRQN 31

#define REG_ALIAS_RW_BITS  (0x0u << 12u)
#define REG_ALIAS_XOR_BITS (0x1u << 12u)
#define REG_ALIAS_SET_BITS (0x2u << 12u)
#define REG_ALIAS_CLR_BITS (0x3u << 12u)

#define hw_set_alias(addr) (*((volatile uint32_t *)(REG_ALIAS_SET_BITS | ((uintptr_t)(&addr)))))
#define hw_clear_alias(addr) (*((volatile uint32_t *)(REG_ALIAS_CLR_BITS | ((uintptr_t)(&addr)))))
#define hw_xor_alias(addr) (*((volatile uint32_t *)(REG_ALIAS_XOR_BITS | ((uintptr_t)(&addr)))))

static __always_inline inline uint32_t disable_interrupts(void)
{
	uint32_t primask = __get_PRIMASK();
	__set_PRIMASK(1);
	return primask;
}

static __always_inline inline void enable_interrupts(uint32_t primask)
{
	__set_PRIMASK(primask);
}

#endif
