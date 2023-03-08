#ifndef _IRQ_H_
#define _IRQ_H_

#include <assert.h>
#include <stdbool.h>

#include <cmsis/cmsis.h>

#ifndef __LAST_IRQN
	#define __LAST_IRQN 239
#endif
#define IRQ_NUM (__LAST_IRQN + 16 + 1)

typedef void (*irq_handler_t)(IRQn_Type irq, void *context);

void irq_register(IRQn_Type irq, uint32_t priority, irq_handler_t handler, void *context);
void irq_unregister(IRQn_Type irq, irq_handler_t handler);

irq_handler_t irq_get_handler(IRQn_Type irq);
void irq_set_handler(IRQn_Type irq, irq_handler_t handler);

void *irq_get_context(IRQn_Type irq);
void irq_set_context(IRQn_Type irq, void *context);

static inline void irq_set_priority(IRQn_Type irq, uint32_t priority)
{
	assert(irq <= __LAST_IRQN);
	NVIC_SetPriority(irq, priority);
}

static inline uint32_t irq_get_priority(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	return NVIC_GetPriority(irq);
}

static inline void irq_enable(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	NVIC_EnableIRQ(irq);
}

static inline void irq_disable(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	NVIC_DisableIRQ(irq);
}

static inline bool irq_is_pending(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	return NVIC_GetPendingIRQ(irq);
}

static inline bool irq_is_enabled(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	return NVIC_GetEnableIRQ(irq);
}

static inline void irq_trigger(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	NVIC_SetPendingIRQ(irq);
	__DMB();
}

static inline void irq_clear(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	NVIC_ClearPendingIRQ(irq);
}

#endif
