/*
 * hal-rp2040-multicore.c
 *
 *  Created on: Apr 24, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <compiler.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>

#include <unistd.h>

#include <init/init-sections.h>

#include <sys/delay.h>
#include <sys/syslog.h>
#include <sys/irq.h>
#include <sys/swi.h>

#include <cmsis/cmsis.h>

#include <hal/rp2040/hal-rp2040-multicore.h>

#define HAL_MULTICORE_MARKER 0x19630209

struct hal_multicore_core
{
	uint32_t marker;
	uint32_t state;
	uintptr_t stack_pointer;
	struct hal_multicore_executable executable;
	struct {
		hal_multicore_event_handler_t func;
		uint32_t mask;
		void *context;
	} event_handlers[HAL_MULTICORE_NUM_EVENT_HANDLERS];
};

extern void _init_tls(void *__tls_block);
extern void _set_tls(void *tls);

void _multicore_start(void);

static atomic_uint multicore_critical_section = UINT32_MAX;
static uint32_t multicore_critical_section_counter = 0;
static struct hal_multicore_core **cores = 0;

static void hal_multicore_flush(uint32_t core)
{
	/* Drain the fifo if it has not been emptied and clear any errors */
	SIO->FIFO_ST = SIO_FIFO_ST_ROE_Msk | SIO_FIFO_ST_WOF_Msk;
	while ((SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk) != 0)
		(void)SIO->FIFO_RD;
}

static void core_dispatch_event(uint32_t event)
{
	struct hal_multicore_core *core = cores[SystemCurrentCore()];
	for (size_t i = 0; i < HAL_MULTICORE_NUM_EVENT_HANDLERS; ++i)
		if ((core->event_handlers[i].mask & event) != 0)
			core->event_handlers[i].func(event, core->event_handlers[i].context);
}

static void core_pend_irq(IRQn_Type irq)
{
	if (irq <= SysTick_IRQn) {

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

			/* Drop everything else */
			default:
				break;
		}

		/* We handler the system irqs */
		return;
	}

	/* NVIC interrupt */
	irq_trigger(irq);
}

static void core_enable_irq(IRQn_Type irq, uint32_t priority)
{
	/* Drop negative irqs */
	if (irq <= SysTick_IRQn)
		return;

	/* Forward */
	irq_enable(irq);

}

static void core_disable_irq(IRQn_Type irq)
{
	/* Drop negative irqs */
	if (irq <= SysTick_IRQn)
		return;

	/* Forward */
	irq_disable(irq);
}

static void core_irq_handler(IRQn_Type irq, void *context)
{
	uint32_t core = SystemCurrentCore();
	uint32_t cmd;

	while (hal_multicore_is_data_avail(core)) {

		/* There will alway be data */
		hal_multicore_recv(core, &cmd, 0);

		/* Handle the command */
		switch (cmd & HAL_MULTICORE_COMMAND_MSK) {
			case HAL_MUILTCORE_EVENT:
				core_dispatch_event(cmd & ~HAL_MULTICORE_COMMAND_MSK);
				break;

			case HAL_MULTICORE_PEND_IRQ:
				core_pend_irq((cmd & ~HAL_MULTICORE_COMMAND_MSK) - 16);
				break;

			case HAL_MULTICORE_IRQ_ENABLE:
				core_enable_irq((cmd & ~HAL_MULTICORE_COMMAND_MSK) - 16, 0);
				break;

			case HAL_MULTICORE_IRQ_DISABLE:
				core_disable_irq((cmd & ~HAL_MULTICORE_COMMAND_MSK) - 16);
				break;

			/* Dropped */
			case HAL_MULTICORE_STATE_CHANGED:
			case HAL_MULTICORE_EXECUTE:
			default:
				break;
		}
	}

	hal_multicore_flush(SystemCurrentCore());
}

bool hal_multicore_is_data_avail(uint32_t core)
{
	assert(core < SystemNumCores);

	return (SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk) != 0;
}

bool hal_multicore_is_space_avail(uint32_t core)
{
	assert(core < SystemNumCores);

	return (SIO->FIFO_ST & SIO_FIFO_ST_RDY_Msk) != 0;
}

int hal_multicore_send(uint32_t core, uint32_t item, uint32_t msec)
{
	assert(core < SystemNumCores);

	/* Wait for space TODO implement timeout */
	while (!hal_multicore_is_space_avail(core))
		__WFE();

	/* Send it on the way */
	SIO->FIFO_WR = item;

	/* Kick the other side */
	__SEV();

	/* All good */
	return 0;
}

int hal_multicore_recv(uint32_t core, uint32_t *item, uint32_t msec)
{
	assert(core < SystemNumCores);

	/* Wait for space TODO implement timeout */
	while (!hal_multicore_is_data_avail(core))
		__WFE();

	/* Get the request */
	*item = SIO->FIFO_RD;

	/* Kick the other side to left them know there is space */
	__SEV();

	/* All good */
	return 0;
}

__noreturn void _multicore_start(void)
{
	struct hal_multicore_executable *multicore;
	struct hal_multicore_core *core = cores[SystemCurrentCore()];
	void *tls_block = alloca((size_t)&__tls_size);;

	/* Initialize the irq and swi subsystems on this core */
	irq_init();
	swi_init();

	/* Bind to the interrupt handler */
	irq_register(SIO_IRQ_PROC0_IRQn + SystemCurrentCore(), INTERRUPT_NORMAL, core_irq_handler, &cores[SystemCurrentCore()]);

	/* Enter the idle state */
	core->state = HAL_MULTICORE_IDLE;

	while (true) {

		/* No interrupts while we wait for an executable */
		irq_disable(SIO_IRQ_PROC0_IRQn + SystemCurrentCore());

		/* First wait for the multicore pointer */
		hal_multicore_recv(0, (uint32_t *)&multicore, UINT32_MAX);

		/* Echo the address back */
		hal_multicore_send(0, (uint32_t)multicore, UINT32_MAX);

		/* Initialize the tls block */
		_init_tls(tls_block);

		/* Bind the initial TLS block */
		_set_tls(tls_block);

		/* Mark as running */
		core->state = HAL_MULTICORE_RUNNING;

		/* Cleanup the fifo */
		hal_multicore_flush(SystemCurrentCore());

		/* Allow interrupt while executable runs */
		irq_enable(SIO_IRQ_PROC0_IRQn + SystemCurrentCore());

		/* Forward to the next stage */
		multicore->status = multicore->entry_point(multicore->argc, multicore->argv);

		/* Mark as idle */
		core->state = HAL_MULTICORE_IDLE;
	}

	/* Disable the interrupt handler */
	irq_unregister(SIO_IRQ_PROC0_IRQn + SystemCurrentCore(), core_irq_handler);

	/* Mark as inside the bootrom */
	core->state = HAL_MULTICORE_BOOTROM;

	/* Set core so it is running in the bootrom */
 	SystemExitCore();
}

void hal_multicore_register_handler(hal_multicore_event_handler_t handler, uint32_t mask, void *context)
{
	int i;
	struct hal_multicore_core *core = cores[SystemCurrentCore()];

	/* Look for an empty slot */
	uint32_t state = disable_interrupts();
	for (i = 0; i < HAL_MULTICORE_NUM_EVENT_HANDLERS; ++i) {
		if (core->event_handlers[i].func == 0) {
			core->event_handlers[i].func = handler;
			core->event_handlers[i].mask = mask;
			core->event_handlers[i].context = context;
			break;
		}
	}
	enable_interrupts(state);

	/* Did we fail */
	if (i == HAL_MULTICORE_NUM_EVENT_HANDLERS)
		syslog_fatal("failed to find multicore event handler slot\n");
}

void hal_multicore_unregister_handler(hal_multicore_event_handler_t handler)
{
	struct hal_multicore_core *core = cores[SystemCurrentCore()];

	/* Look for an matching slot */
	uint32_t state = disable_interrupts();
	for (int i = 0; i < HAL_MULTICORE_NUM_EVENT_HANDLERS; ++i) {
		if (core->event_handlers[i].func == handler) {
			core->event_handlers[i].func = handler;
			core->event_handlers[i].mask = 0;
			core->event_handlers[i].context = 0;
		}
	}
	enable_interrupts(state);
}

void hal_multicore_pend_irq(uint32_t core, IRQn_Type irq)
{
	assert(core < SystemNumCores);
	hal_multicore_send(core, HAL_MULTICORE_PEND_IRQ | ((irq + 16) & ~HAL_MULTICORE_COMMAND_MSK), 0);
}

void hal_multicore_post_event(uint32_t core, uint32_t event)
{
	assert(core < SystemNumCores);

	hal_multicore_send(core, HAL_MUILTCORE_EVENT | (event & ~HAL_MULTICORE_COMMAND_MSK), 0);
}

int hal_multicore_reset(uint32_t core)
{
	assert(core != 0 && core < SystemNumCores);

	/* Power cycle the core */
	SystemResetCore(core);

	/* Update the state */
	cores[core]->state = HAL_MULTICORE_BOOTROM;

	/* Launch the hal multicore startup routine */
	SystemLaunchCore(core, SCB->VTOR, (uintptr_t)cores[core]->stack_pointer, _multicore_start);

	/* Wait for the core to reach the idle state */
	while (cores[core]->state != HAL_MULTICORE_IDLE);

	/* Launch the core executable if available */
	if (cores[core]->executable.entry_point)
		return hal_multicore_start(core, &cores[core]->executable);

	/* Otherwise all done */
	return 0;
}

int hal_multicore_start(uint32_t core, const struct hal_multicore_executable *executable)
{
	uint32_t echo;
	int status = 0;

	assert(core != 0 && core < SystemNumCores && executable != 0);

	/* Make sure the core is idle */
	if (cores[core]->state != HAL_MULTICORE_IDLE) {
		errno = EBUSY;
		return -errno;
	}

	/* No interrupts */
	irq_disable(SIO_IRQ_PROC0_IRQn);

	/* Save the executable */
	cores[core]->executable = *executable;

	/* Send the multicore pointer */
	hal_multicore_send(core, (uint32_t)&cores[core]->executable, UINT32_MAX);

	/* Verify the echoed pointer */
	hal_multicore_recv(core, &echo, UINT32_MAX);

	/* Validate the error */
 	if (echo != (uint32_t)&cores[core]->executable) {
 		errno = EINVAL;
 		status = -errno;
 	}

	/* Cleanup the fifo */
	hal_multicore_flush(SystemCurrentCore());

	/* Let the interrupts fly */
	irq_enable(SIO_IRQ_PROC0_IRQn);

 	/* All good the other core should be running */
 	return status;
}

int hal_multicore_stop(uint32_t core)
{
	assert(core != 0 && core < SystemNumCores);

	/* Clear the executable */
	memset(&cores[core]->executable, 0, sizeof(cores[core]->executable));

	/* Force a reset */
	return hal_multicore_reset(core);
}

uint32_t hal_multicore_enter_critical(void)
{
	uint32_t state = disable_interrupts();
	uint32_t expected = UINT32_MAX;
	while (!atomic_compare_exchange_strong(&multicore_critical_section, &expected, SystemCurrentCore())) {
		if (expected == SystemCurrentCore()) {
			++multicore_critical_section_counter;
			return state;
		}

		/* Wait for the release */
		__WFE();
	}

	/* Initialize the counter */
	multicore_critical_section_counter = 1;

	/* Return the interrupt state */
	return state;
}

void hal_multicore_exit_critical(uint32_t state)
{
	/* Handle nested critical sections */
	if (atomic_load(&multicore_critical_section) == SystemCurrentCore() && multicore_critical_section_counter > 1) {
		--multicore_critical_section_counter;
		return;
	}

	/* Release the critical section */
	atomic_store(&multicore_critical_section, UINT32_MAX);

	/* Kick the other cores */
	__SEV();

	/* All the interrupts */
	enable_interrupts(state);
}

static void hal_multicore_exit_handler(void)
{
	SystemResetCore(1);
}

static __unused void hal_multicore_init(void)
{
	assert(SystemCurrentCore() == 0);

	/* Get some memory the the core pointers */
	cores = sbrk(sizeof(struct hal_multicore_core *) * SystemNumCores);
	if (!cores)
		syslog_fatal("could not allocate multicores\n");

	/* Setup core 0 */
	cores[0] = sbrk(sizeof(struct hal_multicore_core));
	if (!cores[0])
		syslog_fatal("could not allocate core 0\n");
	memset(cores[0], 0, sizeof(struct hal_multicore_core));
	cores[0]->marker = HAL_MULTICORE_MARKER;
	cores[0]->state = HAL_MULTICORE_RUNNING;
	irq_register(SIO_IRQ_PROC0_IRQn, INTERRUPT_NORMAL, core_irq_handler, &cores[0]);

	/* Initialize the other cores */
	for (uint32_t i = 1; i < SystemNumCores; ++i) {

		/* Allocate the core, we only need a stack for cores other than the startup core */
		cores[i] = sbrk(sizeof(struct hal_multicore_core) + HAL_MULTICORE_STACK_SIZE);
		if (!cores[i])
			syslog_fatal("could not allocate core %lu\n", i);
		memset(cores[i], 0, sizeof(struct hal_multicore_core));
		cores[i]->marker = HAL_MULTICORE_MARKER + i;

		/* Initialize the stack pointer */
		cores[i]->stack_pointer = ((uintptr_t)cores[i]) + sizeof(struct hal_multicore_core) + HAL_MULTICORE_STACK_SIZE;

		/* Initialize the state */
		cores[i]->state = HAL_MULTICORE_BOOTROM;

		/* Reset the core */
		SystemResetCore(i);

		/* Launch the hal multicore startup routine */
		SystemLaunchCore(i, SCB->VTOR, (uintptr_t)cores[i]->stack_pointer, _multicore_start);

		/* Wait for the core to reach the idle state */
		while (cores[i]->state != HAL_MULTICORE_IDLE);

		/* Log core initialization */
		syslog_info("core %lu initialized\n", i);
	}

	/* Cleanup the fifo */
	hal_multicore_flush(SystemCurrentCore());

	/* Enable interrupts from the other cores */
	irq_enable(SIO_IRQ_PROC0_IRQn);

	/* Register an exit handler */
	atexit(hal_multicore_exit_handler);
}
PREINIT_PLATFORM_WITH_PRIORITY(hal_multicore_init, HAL_PLATFORM_PRIORITY);
