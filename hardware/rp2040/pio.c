/*
 * rp2040-pio.c
 *
 *  Created on: Jan 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <stdlib.h>
#include <compiler.h>

#include <sys/irq.h>

#include <init/init-sections.h>
#include <hardware/rp2040/pio.h>

typedef PIO0_Type PIO_Type;

struct pio_interrupt_handler
{
	uint32_t mask;
	pio_interrupt_handler_t handler;
	void *context;
	IRQn_Type irq;
};

static void pio_default_irq_handler(uint32_t machine, uint32_t source, void *context);

static struct pio_interrupt_handler pio_handlers[] = { [0 ... PIO_NUM_MACHINES - 1] = {.handler = pio_default_irq_handler, .context = 0 } };

static inline PIO_Type *irq_to_pio(IRQn_Type irq)
{
	return irq <= PIO0_IRQ_1_IRQn ? PIO0 : PIO1;
}

static inline void *machine_to_pio(uint32_t machine)
{
	return machine < (PIO_NUM_MACHINES >> 1) ? PIO0 : PIO1;
}

static inline uint32_t machine_to_mask(uint32_t machine, uint32_t pos)
{
	return (1UL << ((machine & 0x3) + pos));
}

static void pio_default_irq_handler(uint32_t machine, uint32_t source, void *context)
{
	pio_clear_irq(machine, source);
}

static void pio_irq_handler(IRQn_Type irq, void *context)
{
	struct pio_interrupt_handler *current;
	struct pio_interrupt_handler *end;
	uint32_t source;

	assert(context != 0);

	/* Get and clear the status */
	volatile uint32_t *ints = context;
	uint32_t status = *ints;
	clear_mask(&irq_to_pio(irq)->IRQ, status >> 8UL);

	/* Dispatch and interest handlers */
	current = irq <= PIO0_IRQ_1_IRQn ? &pio_handlers[0] : &pio_handlers[PIO_NUM_MACHINES / 2];
	end = current + PIO_NUM_MACHINES / 2;
	while (current < end) {
		source = status & current->mask;
		if (source)
			current->handler(current - pio_handlers, source, current->context);
		++current;
	}
}

struct pio_state_machine *pio_get_machine(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);

	return  machine_to_pio(machine) + offsetof(PIO0_Type, SM0_CLKDIV) + (machine & 0x3) * sizeof(struct pio_state_machine);
}

uint32_t pio_get_rx_dreg(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);
	return (machine & 0x4) ? machine + 12 : machine + 4;
}

uint32_t pio_get_tx_dreg(uint32_t machine)
{
	return (machine & 0x4) ? machine + 8 : machine;
}

void pio_register_machine(uint32_t machine, IRQn_Type irq, pio_interrupt_handler_t handler, void *context)
{
	assert(machine < PIO_NUM_MACHINES && irq >= PIO0_IRQ_0_IRQn && irq <= PIO1_IRQ_1_IRQn);

	pio_handlers[machine].handler = handler;
	pio_handlers[machine].context = context;
	pio_handlers[machine].mask = 0;
	pio_handlers[machine].irq = irq;
}

void pio_unregister_machine(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);

	pio_handlers[machine].handler = pio_default_irq_handler;
	pio_handlers[machine].context = 0;
	pio_handlers[machine].mask = 0;
	pio_handlers[machine].irq = 0;
}

void pio_enable_irq(uint32_t machine, uint32_t source)
{
	assert(machine < PIO_NUM_MACHINES);

	pio_handlers[machine].mask = source & 0x00000fff;
	switch (pio_handlers[machine].irq) {

		case PIO0_IRQ_0_IRQn:
			set_mask(&PIO0->IRQ0_INTE, source & 0x00000fff);
			break;

		case PIO0_IRQ_1_IRQn:
			set_mask(&PIO0->IRQ1_INTE, source & 0x00000fff);
			break;

		case PIO1_IRQ_0_IRQn:
			set_mask(&PIO1->IRQ0_INTE, source & 0x00000fff);
			break;

		case PIO1_IRQ_1_IRQn:
			set_mask(&PIO1->IRQ1_INTE, source & 0x00000fff);
			break;

		default:
			abort();
	}
}

void pio_disable_irq(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);

	switch (pio_handlers[machine].irq) {

		case PIO0_IRQ_0_IRQn:
			clear_mask(&PIO0->IRQ0_INTE, pio_handlers[machine].mask);
			break;

		case PIO0_IRQ_1_IRQn:
			clear_mask(&PIO0->IRQ1_INTE, pio_handlers[machine].mask);
			break;

		case PIO1_IRQ_0_IRQn:
			clear_mask(&PIO1->IRQ0_INTE, pio_handlers[machine].mask);
			break;

		case PIO1_IRQ_1_IRQn:
			clear_mask(&PIO1->IRQ1_INTE, pio_handlers[machine].mask);
			break;

		default:
			abort();
	}

	pio_handlers[machine].mask = 0;
}

void pio_clear_irq(uint32_t machine, uint32_t source)
{
	assert(machine < PIO_NUM_MACHINES);

	PIO0_Type *pio = machine_to_pio(machine);
	clear_mask(&pio->IRQ, (source & 0x00000fff) >> PIO0_IRQ0_INTS_SM0_Pos);
}

void pio_machine_set_clock(uint32_t machine, uint32_t rate_hz)
{
	assert(machine < PIO_NUM_MACHINES && rate_hz <= SystemCoreClock);

	/* Calculate the integer and fractional parts */
	float div = SystemCoreClock / (float)rate_hz;
	uint32_t div_int = div;
	uint32_t div_frac = (div - div_int) * 256;

	/* Set the clock */
	pio_get_machine(machine)->clkdiv = (div_int << PIO0_SM0_CLKDIV_INT_Pos) | (div_frac << PIO0_SM0_CLKDIV_FRAC_Pos);
}

uint32_t pio_machine_get_clock(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);

	/* Get the integer and fractional parts */
	uint32_t clkdiv = pio_get_machine(machine)->clkdiv;
	uint32_t div_int = (clkdiv & PIO0_SM0_CLKDIV_INT_Msk) >> PIO0_SM0_CLKDIV_INT_Pos;
	uint32_t div_frac = (clkdiv & PIO0_SM0_CLKDIV_FRAC_Msk) >> PIO0_SM0_CLKDIV_FRAC_Pos;

	return SystemCoreClock / (div_int + (div_frac / 256.0));
}

void pio_machine_enable(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);

	PIO0_Type *pio = machine_to_pio(machine);
	set_bit(&pio->CTRL, machine);
}

void pio_machine_disable(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);

	PIO0_Type *pio = machine_to_pio(machine);
	clear_bit(&pio->CTRL, machine);
}

bool pio_machine_is_enabled(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);

	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->CTRL & (1UL << (machine >> 1))) != 0;
}

void pio_join_rx_fifo(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);
	clear_bit(&pio_get_machine(machine)->shiftctrl, PIO0_SM0_SHIFTCTRL_FJOIN_TX_Pos);
	set_bit(&pio_get_machine(machine)->shiftctrl, PIO0_SM0_SHIFTCTRL_FJOIN_RX_Pos);
}

void pio_join_tx_fifo(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);
	clear_bit(&pio_get_machine(machine)->shiftctrl, PIO0_SM0_SHIFTCTRL_FJOIN_RX_Pos);
	set_bit(&pio_get_machine(machine)->shiftctrl, PIO0_SM0_SHIFTCTRL_FJOIN_TX_Pos);
}

void pio_break_fifo(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);
	clear_mask(&pio_get_machine(machine)->shiftctrl, PIO0_SM0_SHIFTCTRL_FJOIN_RX_Pos | PIO0_SM0_SHIFTCTRL_FJOIN_TX_Pos);
}

volatile uint32_t *pio_get_rx_fifo(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);
	return machine_to_pio(machine) + offsetof(PIO0_Type, RXF0) + (machine & 0x3) * sizeof(uint32_t);
}

volatile uint32_t *pio_get_tx_fifo(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);
	return machine_to_pio(machine) + offsetof(PIO0_Type, TXF0) + (machine & 0x3) * sizeof(uint32_t);
}

void pio_load_program(uint32_t machine, const uint16_t *program, size_t size, uint16_t origin)
{
	assert(machine < PIO_NUM_MACHINES && program != 0 && size <= 32 && (size - 1) + origin < 32);

	PIO0_Type *pio = machine_to_pio(machine);
	volatile uint32_t *mem = &pio->INSTR_MEM0 + origin;
	for (size_t i = 0; i < size; ++i)
		mem[i] = program[i];
}

uint32_t pio_get_rx_level(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->FLEVEL >> ((machine & 0x3) * 8 + 4)) & 0xf;
}

uint32_t pio_get_tx_level(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->FLEVEL >> ((machine & 0x3) * 8)) & 0xf;
}

bool pio_rx_empty(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->FSTAT & machine_to_mask(machine, PIO0_FSTAT_RXEMPTY_Pos)) != 0;
}

bool pio_rx_full(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->FSTAT & machine_to_mask(machine, PIO0_FSTAT_RXFULL_Pos)) != 0;
}

bool pio_tx_empty(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->FSTAT & machine_to_mask(machine, PIO0_FSTAT_TXEMPTY_Pos)) != 0;
}

bool pio_tx_full(uint32_t machine)
{
	assert(machine < PIO_NUM_MACHINES);
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->FSTAT & machine_to_mask(machine, PIO0_FSTAT_TXFULL_Pos)) != 0;
}

bool pio_is_sticky(uint32_t machine, uint32_t mask)
{
	assert(machine < PIO_NUM_MACHINES);
	PIO0_Type *pio = machine_to_pio(machine);
	return (pio->IRQ & mask) != 0;
}

void pio_clear_sticky(uint32_t machine, uint32_t mask)
{
	assert(machine < PIO_NUM_MACHINES);
	PIO0_Type *pio = machine_to_pio(machine);
	pio->IRQ = mask;
}

void pio_execute(uint32_t machine, uint16_t instr)
{
	assert(machine < PIO_NUM_MACHINES);
	pio_get_machine(machine)->instr = instr;
}

static void pio_init(void)
{
	irq_register(PIO0_IRQ_0_IRQn, INTERRUPT_NORMAL, pio_irq_handler, (void *)&PIO0->IRQ0_INTS);
	irq_register(PIO0_IRQ_1_IRQn, INTERRUPT_NORMAL, pio_irq_handler, (void *)&PIO0->IRQ1_INTS);

	irq_register(PIO1_IRQ_0_IRQn, INTERRUPT_NORMAL, pio_irq_handler, (void *)&PIO1->IRQ0_INTS);
	irq_register(PIO1_IRQ_1_IRQn, INTERRUPT_NORMAL, pio_irq_handler, (void *)&PIO1->IRQ1_INTS);

	irq_enable(PIO0_IRQ_0_IRQn);
	irq_enable(PIO0_IRQ_1_IRQn);

	irq_enable(PIO1_IRQ_0_IRQn);
	irq_enable(PIO1_IRQ_1_IRQn);
}
PREINIT_PLATFORM_WITH_PRIORITY(pio_init, HARDWARE_PLATFORM_INIT_PRIORITY);
