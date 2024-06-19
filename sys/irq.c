#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include <compiler.h>
#include <init/init-sections.h>
#include <sys/irq.h>

struct irq_entry
{
	irq_handler_t handler;
	void *context;
};

static __isr_section __optimize void irq_default_handler(IRQn_Type irq, void *context)
{
}

struct irq_entry irq_dispatch[IRQ_NUM] = { [0 ... IRQ_NUM - 1] = { .handler = irq_default_handler, .context = 0 } };

__weak void irq_register(IRQn_Type irq, uint32_t priority, irq_handler_t handler, void *context)
{
	assert(irq <= __LAST_IRQN);
	irq_disable(irq);
	irq_set_priority(irq, priority);
	irq_set_context(irq, context);
	irq_set_handler(irq, handler);
}

__weak void irq_unregister(IRQn_Type irq, irq_handler_t __unused handler)
{
	assert(irq <= __LAST_IRQN);
	irq_disable(irq);
	irq_set_handler(irq, irq_default_handler);
	irq_set_context(irq, 0);
	irq_set_priority(irq, 0);
}

__weak irq_handler_t irq_get_handler(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	return irq_dispatch[irq + 16].handler;
}

__weak void irq_set_handler(IRQn_Type irq, irq_handler_t handler)
{
	assert(irq <= __LAST_IRQN);
	irq_dispatch[irq + 16].handler = handler ? handler : irq_default_handler;
}

__weak void *irq_get_context(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	return irq_dispatch[irq + 16].context;
}

__weak void irq_set_context(IRQn_Type irq, void *context)
{
	assert(irq <= __LAST_IRQN);
	irq_dispatch[irq + 16].context = context;
}

__weak void irq_set_priority(IRQn_Type irq, uint32_t priority)
{
	assert(irq <= __LAST_IRQN);
	NVIC_SetPriority(irq, priority);
}

__weak uint32_t irq_get_priority(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	return NVIC_GetPriority(irq);
}

__weak void irq_set_affinity(IRQn_Type irq, uint32_t core)
{
}

__weak uint32_t irq_get_affinity(IRQn_Type irq)
{
	return 0;
}

__weak bool irq_is_pending(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	return NVIC_GetPendingIRQ(irq);
}

__weak bool irq_is_enabled(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	return NVIC_GetEnableIRQ(irq);
}

__weak void irq_enable(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	NVIC_ClearPendingIRQ(irq);
	NVIC_EnableIRQ(irq);
}

__weak void irq_disable(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	NVIC_DisableIRQ(irq);
}

__weak void irq_trigger(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	NVIC_SetPendingIRQ(irq);
	__DMB();
}

__weak void irq_clear(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	NVIC_ClearPendingIRQ(irq);
}

__weak void irq_init(void)
{
	/* Set priority for all device interrupts */
	for (uint32_t irq = 0; irq <= __LAST_IRQN; ++irq) {
		NVIC_DisableIRQ(irq);
		NVIC_ClearPendingIRQ(irq);
		NVIC_SetPriority(irq, INTERRUPT_NORMAL);
	}

	__enable_irq();
}
PREINIT_SYSINIT_WITH_PRIORITY(irq_init, IRQ_SYSTEM_INIT_PRIORIY);
