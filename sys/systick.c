#include <errno.h>
#include <config.h>
#include <compiler.h>

#include <init/init-sections.h>

#include <cmsis/cmsis.h>

#include <sys/tls.h>
#include <sys/irq.h>
#include <sys/systick.h>

struct systick_handler
{
	void (*handler)(void *context);
	void *context;
};

extern __weak void (*__systick_array_start[])(void);
extern __weak void (*__systick_array_end[])(void);

core_local volatile unsigned long systicks = 0;
core_local struct systick_handler systick_handlers[SYSTICK_MAX_MANAGED_HANDLERS] =  { 0 };

void SysTick_Handler(void);

__isr_section void SysTick_Handler(void)
{
	/* Clear the Overflow */
	__unused uint32_t value = SysTick->CTRL;

	/* Update the tick count */
	cls_datum(systicks) = cls_datum(systicks) + 1;

	/* Dispatch */
	struct systick_handler *handlers = cls_datum(systick_handlers);
	for (size_t i = 0; i < SYSTICK_MAX_MANAGED_HANDLERS; ++i)
		if (handlers[i].handler != 0)
			handlers[i].handler(handlers[i].context);

#if 0
	/* Dispatch, only the first core dispatch other handlers  */
	for (int i = 0; i < __systick_array_end - __systick_array_start; ++i)
		__systick_array_start[i]();
#endif
}

unsigned long systick_get_ticks(void)
{
	return cls_datum(systicks);
}

void systick_delay(unsigned long ticks)
{
	uint32_t start = systick_get_ticks();
	while((systick_get_ticks() - start) < ticks);
}

int systick_register_handler(void (*handler)(void *context), void *context)
{
	int status = -ENOSPC;

	uint32_t state = disable_interrupts();
	struct systick_handler *handlers = cls_datum(systick_handlers);
	for (size_t i = 0; i < SYSTICK_MAX_MANAGED_HANDLERS; ++i)
		if (handlers[i].handler == 0) {
			handlers[i].handler = handler;
			handlers[i].context = context;
			status = 0;
			break;
		}
	enable_interrupts(state);

	if (status != 0)
		errno = -ENOSPC;

	return status;
}

void systick_unregister_handler(void (*handler)(void *context))
{
	uint32_t state = disable_interrupts();
	struct systick_handler *handlers = cls_datum(systick_handlers);
	for (size_t i = 0; i < SYSTICK_MAX_MANAGED_HANDLERS; ++i)
		if (handlers[i].handler != handler) {
			handlers[i].handler = 0;
			handlers[i].context = 0;
		}
	enable_interrupts(state);
}

void systick_init(void)
{
	/* We might be initialized more than once, skip if so */
	if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
		return;

	/* Set the priority for Systick Interrupt */
	irq_set_priority(SysTick_IRQn, SYSTICK_INTERRUPT_PRIORITY);

	/* Configure the systick */
	SysTick->LOAD  = (SystemCoreClock / 1000) - 1UL;
	SysTick->VAL   = 0UL;
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

	/* Enable tick interrupt */
	irq_enable(SysTick_IRQn);
}
PREINIT_PLATFORM_WITH_PRIORITY(systick_init, SYSTICK_PLATFORM_INIT_PRIORITY);
