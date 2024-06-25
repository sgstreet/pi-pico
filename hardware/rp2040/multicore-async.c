/*
 * multicore.c
 *
 *  Created on: Jun 17, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <string.h>

#include <compiler.h>

#include <sys/async.h>
#include <sys/tls.h>
#include <sys/irq.h>

#include <init/init-sections.h>
#include <cmsis/cmsis.h>

extern char __core_local_tls_1[];

static atomic_uintptr_t multicore_async = 0;

static __noreturn void multicore_async_monitor(void)
{
	/* Re-able the fifo interrupt */
	irq_enable(SIO_IRQ_PROC1_IRQn);

	while (true) {

		/* Wait for some work */
		uintptr_t expected = 0;
		while (atomic_compare_exchange_strong(&multicore_async, &expected, 0))
			__WFE();

		/* Expected contains the async to execute */
		struct async *async = (struct async *)expected;

		/* Initialize the core 1 tls block */
		_init_tls(__core_local_tls_1);
		_set_tls(__core_local_tls_1);

		/* Forward */
		async->func(async);

		/* Mark always done */
		async->done = true;

		/* Clean up */
		multicore_async = 0;

		/* Kick any waiters */
		__SEV();
	}
}

int async_run(struct async *async, async_func_t func, void *context)
{
	assert(async != 0);

	/* Initialize the async */
	memset(async, 0, sizeof(struct async));
	async->context = context;
	async->func = func;

	/* Launch the async */
	multicore_async = (uintptr_t)async;

	/* Kick the other core */
	__SEV();

	/* All good */
	return 0;
}

int async_wait(struct async *async)
{
	assert(async != 0);

	while (!async_is_done(async))
		__WFE();

	if (async_is_canceled(async)) {
		errno = ECANCELED;
		return -ECANCELED;
	}

	return 0;
}

void async_cancel(struct async *async)
{
	uintptr_t expected = 0;

	assert(async != 0);

	/* Request cancel */
	async->cancel = true;

	/* Wait for the cancel to take */
	while (!atomic_compare_exchange_strong(&multicore_async, &expected, 0))
		expected = 0;
}

static void multicore_async_init(void)
{
	assert(SystemCurrentCore == 0);

	/* Ensure the IRQs are disabled */
	irq_disable(SIO_IRQ_PROC0_IRQn);
	irq_disable(SIO_IRQ_PROC1_IRQn);

	/* Launch the monitor */
	SystemLaunchCore(1, SCB->VTOR, (uintptr_t)&__stack_1, multicore_async_monitor);

	/* Wait for the monitor */
	while (!irq_is_enabled(SIO_IRQ_PROC1_IRQn));

	/* Re-enable the interrupt */
	irq_enable(SIO_IRQ_PROC0_IRQn);
}
PREINIT_PLATFORM_WITH_PRIORITY(multicore_async_init, MULTICORE_ASYNC_PLATFORM_INIT_PRIORITY);
