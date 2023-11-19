/*
 * multicore.c
 *
 *  Created on: Oct 10, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <config.h>
#include <compiler.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <unistd.h>
#include <string.h>

#include <board/board.h>

#include <init/init-sections.h>
#include <sys/irq.h>
#include <sys/async.h>

#ifndef MULTICORE_STACK_SIZE
#define MULTICORE_STACK_SIZE 4096
#endif

#define MULTICORE_COMMAND_MSK 0xf0000000

enum multicore_cmd
{
	MULTICORE_EXECUTE_FLASH = 0x10000000,
	MULTICORE_EXECUTE_SRAM = 0x20000000,
	MULTICORE_EVENT = 0x80000000,
	MULTICORE_PEND_IRQ = 0x90000000,
	MULTICORE_CLEAR_IRQ = 0xa0000000,
	MULTICORE_IRQ_ENABLE = 0xb0000000,
	MULTICORE_IRQ_DISABLE = 0xc0000000,
	MULTICORE_SET_PRIORITY = 0xd0000000,
};

enum multicore_state
{
	MULTICORE_BOOTROM = -1,
	MULTICORE_IDLE = 0,
	MULTICORE_RUNNING = 1,
};

void __multicore_init(void);
void multicore_reset(void);
void multicore_post(uintptr_t event);

extern __noreturn void *rom_func_lookup(uint32_t);
extern void _init_tls(void *__tls_block);
extern void _set_tls(void *tls);
extern void *__tls_size;

static atomic_uint irq_core = BOARD_IRQ_BINDING;
static bool irq_enabled[__LAST_IRQN + 1]  = { 0 };

static atomic_bool multicore_run = true;
static atomic_uint multicore_state = MULTICORE_BOOTROM;
static atomic_uintptr_t multicore_async = 0;

static inline uint32_t multicore_current(void)
{
	return SIO->CPUID;
}

void multicore_reset(void)
{
	/* Power off the core */
	set_bit(&PSM->FRCE_OFF, PSM_FRCE_OFF_proc1_Pos);

	/* Wait for it to be powered off */
	while ((PSM->DONE & PSM_DONE_proc1_Msk) != 0);

	/* Power it back on */
	clear_bit(&PSM->FRCE_OFF, PSM_FRCE_OFF_proc1_Pos);

	/* Wait for it to be powered on */
	while ((PSM->DONE & PSM_DONE_proc1_Msk) == 0);

	/* Mark as in the bootrom */
	multicore_state = MULTICORE_BOOTROM;
}

static __noreturn void multicore_exit(void)
{
	/* Mark state */
	multicore_state = MULTICORE_BOOTROM;

	/* Enter the bootrom */
	void (*wait_for_vector)(void) = rom_func_lookup('W' | 'V' << 8);
	wait_for_vector();
}

static bool multicore_is_data_avail(void)
{
	return (SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk) != 0;
}

static bool multicore_is_space_avail(void)
{
	return (SIO->FIFO_ST & SIO_FIFO_ST_RDY_Msk) != 0;
}

static void multicore_send(uint32_t item)
{
	/* Wait for space TODO implement timeout */
	while (!multicore_is_space_avail())
		__WFE();

	/* Send it on the way */
	SIO->FIFO_WR = item;

	/* Kick the other side */
	__SEV();
}

static uint32_t multicore_recv(void)
{
	/* Wait for space TODO implement timeout */
	while (!multicore_is_data_avail())
		__WFE();

	/* Get the request */
	uint32_t item = SIO->FIFO_RD;

	/* Kick the other side to left them know there is space */
	__SEV();

	/* All good */
	return item;
}

static void multicore_flush()
{
	/* Drain the fifo if it has not been emptied and clear any errors */
	SIO->FIFO_ST = SIO_FIFO_ST_ROE_Msk | SIO_FIFO_ST_WOF_Msk;
	while (multicore_is_data_avail())
		(void)SIO->FIFO_RD;

	/* Other core might be waiting */
	__SEV();
}

static void multicore_bootstrap(uintptr_t vector_table, void *stack_pointer, void (*entry_point)(void))
{
	const uint32_t boot_cmds[] = { 0, 0, 1, vector_table, (uintptr_t)stack_pointer, (uintptr_t)entry_point };

	/* Ensure the core in in the bootroom */
	if (multicore_state != MULTICORE_BOOTROM)
		abort();

	/* No PROC 0 interrupts while we launch the core */
	bool irq_state = NVIC_GetEnableIRQ(SIO_IRQ_PROC0_IRQn);
	NVIC_DisableIRQ(SIO_IRQ_PROC0_IRQn);

	/* Clean up the FIFO status */
	SIO->FIFO_ST = SIO_FIFO_ST_WOF_Msk | SIO_FIFO_ST_ROE_Msk;

	/* Run the launch state machine */
	size_t idx = 0;
	do {
		/* Flush the fifo if the command is zero */
		if (boot_cmds[idx] == 0)
			multicore_flush();

		/* Send the command, is the loop really required? */
		multicore_send(boot_cmds[idx]);

		/* Start again if we got a crappy response */
		if (multicore_recv() != boot_cmds[idx++])
			idx = 0;

	} while (idx < array_sizeof(boot_cmds));

	/* Clean up the fifo */
	multicore_flush();

	/* Restore the interrupt state */
	if (irq_state)
		NVIC_EnableIRQ(SIO_IRQ_PROC0_IRQn);
}

static void multicore_pend_irq(IRQn_Type irq)
{
	/* These interrupts are in the interrupt control and status register */
	switch (irq) {

		case NonMaskableInt_IRQn:
			SCB->ICSR = SCB_ICSR_NMIPENDSET_Msk;
			break;

		case PendSV_IRQn:
			SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
			break;

		case SysTick_IRQn:
			SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
			break;

		default:
			NVIC_SetPendingIRQ(irq);
			break;
	}

	/* Ensure the memory write is complete */
	__DMB();
}

static void multicore_clear_irq(IRQn_Type irq)
{
	/* NVIC interrupt */
	NVIC_ClearPendingIRQ(irq);
	__DMB();
}

static void multicore_enable_irq(IRQn_Type irq)
{
	/* Clear any pending interrupt */
	NVIC_ClearPendingIRQ(irq);

	/* Then enable it */
	NVIC_EnableIRQ(irq);

	/* Cached the enable */
	if (irq >= 0)
		irq_enabled[irq] = true;
}

static void multicore_disable_irq(IRQn_Type irq)
{
	/* Forward */
	NVIC_DisableIRQ(irq);

	/* Cached the enable */
	if (irq >= 0)
		irq_enabled[irq] = false;
}

static void multicore_set_priority(IRQn_Type irq, uint32_t priority)
{
	/* Set the priority */
	NVIC_SetPriority(irq, priority);
}

static void multicore_irq_handler(void)
{
	uint32_t cmd;

	while (multicore_is_data_avail()) {

		/* There will always be data */
		cmd = multicore_recv();

		/* Handle the command */
		switch (cmd & MULTICORE_COMMAND_MSK) {

			case MULTICORE_EXECUTE_FLASH:
			case MULTICORE_EXECUTE_SRAM: {
				((void (*)(void))cmd)();
				break;
			}

			case MULTICORE_EVENT:
				break;

			case MULTICORE_PEND_IRQ: {
				IRQn_Type irq = (cmd & 0xffff) - 16;
				multicore_pend_irq(irq);
				break;
			}

			case MULTICORE_CLEAR_IRQ: {
				IRQn_Type irq = (cmd & 0xffff) - 16;
				multicore_clear_irq(irq);
				break;
			}

			case MULTICORE_IRQ_ENABLE: {
				IRQn_Type irq = (cmd & 0xffff) - 16;
				multicore_enable_irq(irq);
				break;
			}

			case MULTICORE_IRQ_DISABLE: {
				IRQn_Type irq = (cmd & 0xffff) - 16;
				multicore_disable_irq(irq);
				break;
			}

			case MULTICORE_SET_PRIORITY: {
				IRQn_Type irq = (cmd & 0xffff) - 16;
				uint32_t priority = (cmd >> 16) & 0x00ff;
				multicore_set_priority(irq, priority);
				break;
			}

			default:
				break;
		}
	}

	SIO->FIFO_ST = SIO_FIFO_ST_ROE_Msk | SIO_FIFO_ST_WOF_Msk;
}
__alias("multicore_irq_handler") void SIO_IRQ_PROC0_Handler(void);
__alias("multicore_irq_handler") void SIO_IRQ_PROC1_Handler(void);
__alias("multicore_irq_handler") void NMI_Handler(void);

static __noreturn void multicore_monitor(void)
{
	uintptr_t expected;

	/* We need space the the tls */
	void *tls_block = alloca((size_t)&__tls_size);;

	/* Flush the fifo */
	multicore_flush();

	/* Setup the core zero interrupt handler as an NMI */
	SYSCFG->PROC1_NMI_MASK = (1UL << SIO_IRQ_PROC1_IRQn);

	/* Enter the idle state */
	multicore_state = MULTICORE_IDLE;

	while (multicore_run) {

		/* Busy wait for something to do */
		expected = 0;
		while (atomic_compare_exchange_strong(&multicore_async, &expected, 0))
			__WFE();

		/* Initialize the tls block */
		_init_tls(tls_block);

		/* Bind the initial TLS block */
		_set_tls(tls_block);

		/* Mark as running */
		multicore_state = MULTICORE_RUNNING;

		/* Forward to the next stage */
		struct async *async = (struct async *)expected;
		if (!async_is_canceled(async))
			async->func(async);
		async_done(async);
		multicore_async = 0;
		__SEV();

		/* Mark as idle */
		multicore_state = MULTICORE_IDLE;
	}

	/* Disable the interrupt handler */
	NVIC_DisableIRQ(SIO_IRQ_PROC1_IRQn);

	/* Set core so it is running in the bootrom */
	multicore_exit();
}

void multicore_post(uintptr_t event)
{
	multicore_send(event);
}

void __multicore_init(void)
{
	assert(multicore_current() == 0);

	/* Flush the fifo */
	multicore_flush();

	/* Ensure clean startup by reseting the core */
	multicore_reset();

	/* Launch the replacement core 1 handler */
	multicore_bootstrap((uintptr_t)&__vtor_1, &__stack_1, multicore_monitor);

	/* Flush the fifo */
	multicore_flush();

	/* Setup the core zero interrupt handler as an NMI */
	SYSCFG->PROC0_NMI_MASK = (1UL << SIO_IRQ_PROC0_IRQn);

	/* Make sure interrupts are enabled and the the vtor points into core 0 */
	SCB->VTOR =(uintptr_t)&__vtor_0;
	__enable_irq();
}

void irq_set_priority(IRQn_Type irq, uint32_t priority)
{
	assert(irq <= __LAST_IRQN);

	/* Set the local priority */
	multicore_set_priority(irq, priority);

	/* This is for the other core, send a message */
	multicore_send(MULTICORE_SET_PRIORITY | (priority & 0xffff) << 16 | (irq + 16));
}

uint32_t irq_get_priority(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);

	/* System priority are not cached */
	return NVIC_GetPriority(irq);
}

bool irq_is_enabled(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);

	/* Use the cache */
	if (irq >= 0)
		return irq_enabled[irq];

	/* System irqs are not cached */
	return NVIC_GetEnableIRQ(irq);
}

void irq_enable(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);

	/* Is the irq bound to the current core? */
	if (irq < 0 || irq_get_core(irq) == multicore_current()) {
		multicore_enable_irq(irq);
		return;
	}

	/* This is for the other core, send a message */
	multicore_send(MULTICORE_IRQ_ENABLE | (irq + 16));
}

void irq_disable(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);

	/* Is the irq bound to the current core? */
	if (irq < 0 || irq_get_core(irq) == multicore_current()) {
		multicore_disable_irq(irq);
		return;
	}

	/* This is for the other core, send a message */
	multicore_send(MULTICORE_IRQ_DISABLE | (irq + 16));
}

void irq_trigger(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);

	/* Is the irq bound to the current core? HACK ALERT allow passing core in the second byte of the irq */
	if (irq < 0 || irq_get_core(irq) == multicore_current()) {
		multicore_pend_irq(irq);
		return;
	}

	/* This is for the other core, send a message */
	multicore_send(MULTICORE_PEND_IRQ | (irq + 16));
}

void irq_clear(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);

	/* Is the irq bound to the current core? */
	if (irq < 0 || irq_get_core(irq) == multicore_current()) {
		multicore_clear_irq(irq);
		return;
	}

	/* This is for the other core, send a message */
	multicore_send(MULTICORE_CLEAR_IRQ | irq);
}

void irq_set_core(IRQn_Type irq, uint32_t core)
{
	assert(irq >= 0 && irq <= __LAST_IRQN && core < 2);

	if (core)
		atomic_fetch_or(&irq_core, (1UL << irq));
	else
		atomic_fetch_and(&irq_core, ~(1UL << irq));
}

uint32_t irq_get_core(IRQn_Type irq)
{
	assert(irq >= 0 && irq <= __LAST_IRQN);

	return (irq_core >> irq) & 0x00000001;
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
	__SEV();

	/* All good */
	return 0;
}

void async_wait(struct async *async)
{
	assert(async != 0);

	while (!async_is_done(async))
		__WFE();
}

void async_cancel(struct async *async)
{
	assert(async != 0);

	/* Request cancel */
	async->cancel = true;

	/* Wait for the cancel to take */
	uintptr_t expected = 0;
	while (!atomic_compare_exchange_strong(&multicore_async, &expected, 0))
		__WFE();
}
