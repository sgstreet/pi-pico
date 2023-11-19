#ifndef _CMSIS_H_
#define _CMSIS_H_

#include <stdint.h>

#include <compiler.h>
#include <cmsis/cmsis_compiler.h>
#include <cmsis/rp2040.h>

#define __LAST_IRQN 31

#define IRQ2MASK(irq) (1UL << (irq))

#define REG_ALIAS_RW_BITS  (0x0u << 12u)
#define REG_ALIAS_XOR_BITS (0x1u << 12u)
#define REG_ALIAS_SET_BITS (0x2u << 12u)
#define REG_ALIAS_CLR_BITS (0x3u << 12u)

#define HW_SET_ALIAS(addr) (*((volatile uint32_t *)(REG_ALIAS_SET_BITS | ((uintptr_t)(&addr)))))
#define HW_CLEAR_ALIAS(addr) (*((volatile uint32_t *)(REG_ALIAS_CLR_BITS | ((uintptr_t)(&addr)))))
#define HW_XOR_ALIAS(addr) (*((volatile uint32_t *)(REG_ALIAS_XOR_BITS | ((uintptr_t)(&addr)))))

static __always_inline inline void set_bit(volatile void *addr, uint32_t bit)
{
	volatile uint32_t *alias = (volatile uint32_t *)(REG_ALIAS_SET_BITS | ((uintptr_t)addr));
	*alias = (1UL << bit);
}

static __always_inline inline void set_mask(volatile void *addr, uint32_t mask)
{
	volatile uint32_t *alias = (volatile uint32_t *)(REG_ALIAS_SET_BITS | ((uintptr_t)addr));
	*alias = mask;
}

static __always_inline inline void clear_bit(volatile void *addr, uint32_t bit)
{
	volatile uint32_t *alias = (volatile uint32_t *)(REG_ALIAS_CLR_BITS | ((uintptr_t)addr));
	*alias = (1UL << bit);
}

static __always_inline inline void clear_mask(volatile void *addr, uint32_t mask)
{
	volatile uint32_t *alias = (volatile uint32_t *)(REG_ALIAS_CLR_BITS | ((uintptr_t)addr));
	*alias = mask;
}

static __always_inline inline void xor_bit(volatile void *addr, uint32_t bit)
{
	volatile uint32_t *alias = (volatile uint32_t *)(REG_ALIAS_XOR_BITS | ((uintptr_t)addr));
	*alias = (1UL << bit);
}

static __always_inline inline uint32_t mask_reg(volatile void *addr, uint32_t mask)
{
	volatile uint32_t *reg = addr;
	return *reg & mask;
}

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
