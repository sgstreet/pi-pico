/*
 * multicore-event.c
 *
 *  Created on: Jun 19, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <compiler.h>
#include <config.h>

#include <sys/irq.h>

#include <cmsis/cmsis.h>
#include <init/init-sections.h>

#include <hardware/rp2040/multicore-event.h>

#define MULTICORE_EVENT_Pos (__builtin_clz(MULTICORE_NUM_EVENTS - 1))
#define MULTICORE_EVENT_Mask ((MULTICORE_NUM_EVENTS - 1) << MULTICORE_EVENT_Pos)

static void multicore_event_default_handler(uint32_t event, void *context);

static struct
{
	multicore_event_handler_t handler;
	void *context;
} multicore_event_handlers[MULTICORE_NUM_EVENTS] = { [0 ... array_sizeof(multicore_event_handlers) - 1] = { .handler = multicore_event_default_handler, .context = 0} };

static void multicore_event_default_handler(uint32_t event, void *context)
{
}

static bool sio_fifo_is_data_avail(void)
{
	return (SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk) != 0;
}

static bool sio_fifo_is_space_avail(void)
{
	return (SIO->FIFO_ST & SIO_FIFO_ST_RDY_Msk) != 0;
}

void multicore_event_post(uintptr_t event)
{
	/* Wait for space TODO implement timeout */
	while (!sio_fifo_is_space_avail())
		__WFE();

	/* Send it on the way */
	SIO->FIFO_WR = event;

	/* Kick the other side */
	__SEV();
}

static uint32_t sio_fifo_recv(void)
{
	/* Wait for space TODO implement timeout */
	while (!sio_fifo_is_data_avail())
		__WFE();

	/* Get the request */
	uint32_t item = SIO->FIFO_RD;

	/* Kick the other side to left them know there is space */
	__SEV();

	/* All good */
	return item;
}

static void sio_fifo_irq_handler(void)
{
	while (sio_fifo_is_data_avail()) {

		/* There will always be data */
		uint32_t event = sio_fifo_recv();

		/* Extract the id */
		uint32_t event_id = (event & MULTICORE_EVENT_Mask) >> MULTICORE_EVENT_Pos;

		/* Dispatch */
		multicore_event_handlers[event_id].handler(event, multicore_event_handlers[event_id].context);
	}

	/* Clear any errors */
	SIO->FIFO_ST = SIO_FIFO_ST_ROE_Msk | SIO_FIFO_ST_WOF_Msk;
}
__alias("sio_fifo_irq_handler") void SIO_IRQ_PROC0_Handler(void);
__alias("sio_fifo_irq_handler") void SIO_IRQ_PROC1_Handler(void);

void multicore_event_register(uint32_t event, multicore_event_handler_t handler, void *context)
{
	uint32_t event_id = (event & MULTICORE_EVENT_Mask) >> MULTICORE_EVENT_Pos;
	multicore_event_handlers[event_id].context = context;
	multicore_event_handlers[event_id].handler = handler;
}

void multicore_event_unregister(uint32_t event, multicore_event_handler_t handler)
{
	uint32_t event_id = (event & MULTICORE_EVENT_Mask) >> MULTICORE_EVENT_Pos;
	multicore_event_handlers[event_id].handler = multicore_event_default_handler;
	multicore_event_handlers[event_id].context = 0;
}

static void multicore_event_execute(uint32_t event, void *context)
{
	((void (*)(void))event)();
}

static void multicore_event_init(void)
{
	/* Now dance around getting both irqs enabled while not using the fifo before we are done */
	if (SystemCurrentCore == 0) {

		/* Ensure the IRQs are disabled */
		irq_disable(SIO_IRQ_PROC0_IRQn);
		irq_disable(SIO_IRQ_PROC1_IRQn);

		/* Register the execution handlers */
		multicore_event_register(MULTICORE_EVENT_EXECUTE_FLASH, multicore_event_execute, 0);
		multicore_event_register(MULTICORE_EVENT_EXECUTE_SRAM, multicore_event_execute, 0);

		/* Forward to core 1 */
		SystemLaunchCore(1, SCB->VTOR, (uintptr_t)&__stack_1, multicore_event_init);

		/* Wait the the core 1 initialization to complete */
		while (!irq_is_enabled(SIO_IRQ_PROC1_IRQn));

		/* Enable are side */
		irq_set_affinity(SIO_IRQ_PROC0_IRQn, 0);
		irq_set_priority(SIO_IRQ_PROC0_IRQn, INTERRUPT_REALTIME);
		irq_enable(SIO_IRQ_PROC0_IRQn);

	} else {

		/* Enable the fifo interrupt */
		irq_set_affinity(SIO_IRQ_PROC1_IRQn, 1);
		irq_set_priority(SIO_IRQ_PROC1_IRQn, INTERRUPT_REALTIME);
		irq_enable(SIO_IRQ_PROC1_IRQn);

		/* Head back into the ROM */
		SystemExitCore();
	}
}
PREINIT_PLATFORM_WITH_PRIORITY(multicore_event_init, MULTICORE_EVENT_PLATFORM_INIT_PRIORITY);

