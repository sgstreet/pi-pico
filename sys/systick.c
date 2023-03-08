#include <errno.h>

#include <config.h>
#include <compiler.h>

#include <init/init-sections.h>

#include <cmsis/cmsis.h>

#include <sys/systick.h>

extern void (*__systick_array_start[])(void) __attribute__((weak));
extern void (*__systick_array_end[])(void) __attribute__((weak));

static volatile unsigned long systicks = 0;

void SysTick_Handler(void);

__isr_section void SysTick_Handler(void)
{
	/* Clear the Overflow */
	__unused uint32_t value = SysTick->CTRL;

	/* Update the tick count */
	++systicks;

	/* Dispatch */
	for (int i = 0; i < __systick_array_end - __systick_array_start; ++i)
		__systick_array_start[i]();
}

unsigned long systick_get_ticks(void)
{
	return systicks;
}

void systick_delay(unsigned long msecs)
{
	uint32_t start = systick_get_ticks();
	while((systick_get_ticks() - start) < msecs);
}

static void systick_init(void)
{
	/* Set the priority for Systick Interrupt */

	/* Configure the systick */
	SysTick->LOAD  = (SystemCoreClock / 1000) - 1UL;
	SysTick->VAL   = 0UL;
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

	/* Enable tick interrupt */
	NVIC_EnableIRQ(SysTick_IRQn);
}
PREINIT_SYSINIT_WITH_PRIORITY(systick_init, SYSTICK_SYSTEM_INIT_PRIORITY);
