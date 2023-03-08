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
	irq_clear(irq);
}

struct irq_entry irq_dispatch[IRQ_NUM] =  { [0 ... IRQ_NUM - 1] = { .handler = irq_default_handler, .context = 0 } };

void irq_register(IRQn_Type irq, uint32_t priority, irq_handler_t handler, void *context)
{
	assert(irq <= __LAST_IRQN);
	irq_disable(irq);
	irq_set_priority(irq, priority);
	irq_set_context(irq, context);
	irq_set_handler(irq, handler);
}

void irq_unregister(IRQn_Type irq, irq_handler_t __unused handler)
{
	assert(irq <= __LAST_IRQN);
	irq_disable(irq);
	irq_set_handler(irq, irq_default_handler);
	irq_set_context(irq, 0);
	irq_set_priority(irq, 0);
}

irq_handler_t irq_get_handler(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	return irq_dispatch[irq + 16].handler;
}

void irq_set_handler(IRQn_Type irq, irq_handler_t handler)
{
	assert(irq <= __LAST_IRQN);
	irq_dispatch[irq + 16].handler = handler ? handler : irq_default_handler;
}

void *irq_get_context(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);
	return irq_dispatch[irq + 16].context;
}

void irq_set_context(IRQn_Type irq, void *context)
{
	assert(irq <= __LAST_IRQN);
	irq_dispatch[irq + 16].context = context;
}

static void irq_init(void)
{
	/* Set the system priorities */
	NVIC_SetPriority(SysTick_IRQn, SYSTICK_INTERRUPT_PRIORITY);
	NVIC_SetPriority(SVCall_IRQn, SVCALL_INTERRUPT_PRIORITY);
	NVIC_SetPriority(PendSV_IRQn, PENDSV_INTERRUPT_PRIORITY);

	/* Set priority for all device interrupts */
	for (uint32_t irq = 0; irq <= __LAST_IRQN; ++irq)
		NVIC_SetPriority(irq, INTERRUPT_ABOVE_NORMAL);

	/* Ensure that interrupts are enabled, the reset handler disabled them */
	SCB->VTOR = (uint32_t)&__vtor;
	__DSB();
	__enable_irq();
}
PREINIT_SYSINIT_WITH_PRIORITY(irq_init, IRQ_SYSTEM_INIT_PRIORIY);
