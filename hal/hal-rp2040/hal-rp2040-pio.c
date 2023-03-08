/*
 * hal-rp2040-pio.c
 *
 *  Created on: Jan 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>

#include <compiler.h>

#include <sys/irq.h>

#include <init/init-sections.h>
#include <hal/rp2040/hal-rp2040.h>

struct hal_pio_interrupt_handler
{
	uint32_t mask;
	hal_pio_interrupt_handler_t handler;
	void *context;
};

static void hal_pio_default_irq_handler(uint32_t machine, uint32_t source, void *context);

static struct hal_pio_interrupt_handler core0_handlers[] = { [0 ... HAL_NUM_PIO_MACHINES - 1] = {.handler = hal_pio_default_irq_handler, .context = 0 } };
static struct hal_pio_interrupt_handler core1_handlers[] = { [0 ... HAL_NUM_PIO_MACHINES - 1] = {.handler = hal_pio_default_irq_handler, .context = 0 } };

static void hal_pio_default_irq_handler(uint32_t machine, uint32_t source, void *context)
{
	hal_pio_clear_irq(machine, source);
}

static void pio_irq_handler(IRQn_Type irq, void *context)
{
	assert(context != 0);

	struct hal_pio_interrupt_handler *handlers = context;
	PIO0_Type *pio = (irq == PIO0_IRQ_0_IRQn || irq == PIO0_IRQ_1_IRQn) ? PIO0 : PIO1;
	uint32_t status = (irq == PIO0_IRQ_0_IRQn || irq == PIO1_IRQ_0_IRQn) ? pio->IRQ0_INTS : pio->IRQ1_INTS;
	pio->IRQ = status >> PIO0_IRQ0_INTS_SM0_Pos;
	for (size_t i = 0; i < HAL_NUM_PIO_MACHINES; ++i) {
		uint32_t source = status & handlers[i].mask;
		if (source)
			handlers[i].handler(i, source, handlers[i].context);
	}
}

static inline void *machine_to_pio(uint32_t machine)
{
	return machine < (HAL_NUM_PIO_MACHINES / 2) ? PIO0 : PIO1;
}

static inline uint32_t machine_to_mask(uint32_t machine, uint32_t pos)
{
	return (1UL << ((machine & 0x3) + pos));
}

struct hal_pio_machine *hal_pio_get_machine(uint32_t machine)
{
	assert(machine < HAL_NUM_PIO_MACHINES);

	return  machine_to_pio(machine) + offsetof(PIO0_Type, SM0_CLKDIV) + (machine & 0x3) * sizeof(struct hal_pio_machine);
}

uint32_t hal_pio_get_rx_dreg(uint32_t machine)
{
	assert(machine < HAL_NUM_PIO_MACHINES);
	return (machine & 0x4) ? machine + 12 : machine + 4;
}

uint32_t hal_pio_get_tx_dreg(uint32_t machine)
{
	return (machine & 0x4) ? machine + 8 : machine;
}

void hal_pio_register_irq(uint32_t machine, hal_pio_interrupt_handler_t handler, void *context)
{
	assert(machine < HAL_NUM_PIO_MACHINES);

	if (SIO->CPUID) {
		core1_handlers[machine].mask = 0;
		core1_handlers[machine].context = context;
		core1_handlers[machine].handler = handler;
	} else {
		core0_handlers[machine].mask = 0;
		core0_handlers[machine].context = context;
		core0_handlers[machine].handler = handler;
	}
}

void hal_pio_unregister_irq(uint32_t machine)
{
	assert(machine < HAL_NUM_PIO_MACHINES);

	if (SIO->CPUID) {
		hal_pio_disable_irq(machine, core1_handlers[machine].mask);
		core1_handlers[machine].handler = hal_pio_default_irq_handler;
		core1_handlers[machine].context = 0;
		core1_handlers[machine].mask = 0;
	} else {
		hal_pio_disable_irq(machine, core0_handlers[machine].mask);
		core0_handlers[machine].handler = hal_pio_default_irq_handler;
		core0_handlers[machine].context = 0;
		core0_handlers[machine].mask = 0;
	}
}

void hal_pio_enable_irq(uint32_t machine, uint32_t source)
{
	assert(machine < HAL_NUM_PIO_MACHINES);

	PIO0_Type *pio = machine_to_pio(machine);
	if (SIO->CPUID) {
		core1_handlers[machine].mask = source & 0x00000fff;
		hw_set_alias(pio->IRQ1_INTE) = source & 0x00000fff;
	} else {
		core0_handlers[machine].mask = source & 0x00000fff;
		hw_set_alias(pio->IRQ0_INTE) = source & 0x00000fff;
	}
}

void hal_pio_disable_irq(uint32_t machine, uint32_t source)
{
	assert(machine < HAL_NUM_PIO_MACHINES);

	PIO0_Type *pio = machine_to_pio(machine);
	if (SIO->CPUID) {
		hw_clear_alias(pio->IRQ1_INTE) = source & 0x00000fff;
		core1_handlers[machine].mask = source & 0x00000fff;
	} else {
		hw_clear_alias(pio->IRQ0_INTE) = source & 0x00000fff;
		core0_handlers[machine].mask = source & 0x00000fff;
	}
}

void hal_pio_clear_irq(uint32_t machine, uint32_t source)
{
	assert(machine < HAL_NUM_PIO_MACHINES);

	PIO0_Type *pio = machine_to_pio(machine);
	hw_clear_alias(pio->IRQ) = ((source & 0x00000fff) >> PIO0_IRQ0_INTS_SM0_Pos);
}

void hal_pio_machine_enable(uint32_t machine)
{
	assert(machine < HAL_NUM_PIO_MACHINES);

	PIO0_Type *pio = machine_to_pio(machine);
	hw_set_alias(pio->CTRL) = (1UL << machine);
}

void hal_pio_machine_disable(uint32_t machine)
{
	assert(machine < HAL_NUM_PIO_MACHINES);

	PIO0_Type *pio = machine_to_pio(machine);
	hw_clear_alias(pio->CTRL) = (1UL << machine);
}

bool hal_pio_machine_is_enabled(uint32_t machine)
{
	assert(machine < HAL_NUM_PIO_MACHINES);

	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->CTRL & (1UL << (machine >> 1))) != 0;
}

volatile uint32_t *hal_pio_get_rx_fifo(uint32_t machine)
{
	assert(machine < HAL_NUM_PIO_MACHINES);
	return machine_to_pio(machine) + offsetof(PIO0_Type, RXF0) + (machine & 0x3) * sizeof(uint32_t);
}

volatile uint32_t *hal_pio_get_tx_fifo(uint32_t machine)
{
	assert(machine < HAL_NUM_PIO_MACHINES);
	return machine_to_pio(machine) + offsetof(PIO0_Type, TXF0) + (machine & 0x3) * sizeof(uint32_t);
}

void hal_pio_load_program(uint32_t machine, const uint16_t *program, size_t size, uint16_t origin)
{
	assert(machine < HAL_NUM_PIO_MACHINES && program != 0 && size + origin < 32);

	PIO0_Type *pio = machine_to_pio(machine);
	volatile uint32_t *mem = &pio->INSTR_MEM0 + origin;
	for (size_t i = 0; i < size; ++i)
		mem[i + origin] = program[i];
}

uint32_t hal_pio_get_rx_level(uint32_t machine)
{
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->FLEVEL >> ((machine & 0x3) * 8 + 4)) & 0xf;
}

uint32_t hal_pio_get_tx_level(uint32_t machine)
{
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->FLEVEL >> ((machine & 0x3) * 8)) & 0xf;
}

bool hal_pio_rx_empty(uint32_t machine)
{
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->FSTAT & machine_to_mask(machine, PIO0_FSTAT_RXEMPTY_Pos)) != 0;
}

bool hal_pio_rx_full(uint32_t machine)
{
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->FSTAT & machine_to_mask(machine, PIO0_FSTAT_RXFULL_Pos)) != 0;
}

bool hal_pio_tx_empty(uint32_t machine)
{
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->FSTAT & machine_to_mask(machine, PIO0_FSTAT_TXEMPTY_Pos)) != 0;
}

bool hal_pio_tx_full(uint32_t machine)
{
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->FSTAT & machine_to_mask(machine, PIO0_FSTAT_TXFULL_Pos)) != 0;
}

bool hal_pio_is_sticky(uint32_t machine, uint32_t mask)
{
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->IRQ & mask) != 0;
}

void hal_pio_clear_sticky(uint32_t machine, uint32_t mask)
{
	PIO0_Type *pio = machine_to_pio(machine);
	pio->IRQ = mask;
}

static void hal_pio_init(void)
{
	if (SIO->CPUID) {
		irq_register(PIO0_IRQ_1_IRQn, INTERRUPT_ABOVE_NORMAL, pio_irq_handler, core1_handlers);
		irq_register(PIO1_IRQ_1_IRQn, INTERRUPT_ABOVE_NORMAL, pio_irq_handler, core1_handlers);
		irq_enable(PIO0_IRQ_1_IRQn);
		irq_enable(PIO1_IRQ_1_IRQn);
	} else {
		irq_register(PIO0_IRQ_0_IRQn, INTERRUPT_ABOVE_NORMAL, pio_irq_handler, core0_handlers);
		irq_register(PIO1_IRQ_0_IRQn, INTERRUPT_ABOVE_NORMAL, pio_irq_handler, core0_handlers);
		irq_enable(PIO0_IRQ_0_IRQn);
		irq_enable(PIO1_IRQ_0_IRQn);
	}
}
PREINIT_PLATFORM_WITH_PRIORITY(hal_pio_init, HAL_PLATFORM_PRIORITY);


