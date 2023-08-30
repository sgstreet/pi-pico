/*
 * half-duplex.c
 *
 *  Created on: Jan 2, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <config.h>

#include <sys/irq.h>

#include <board/board.h>
#include <devices/half-duplex.h>

#include "half-duplex.pio.h"

#define HALF_DUPLEX_RX_ERROR 0x01
#define HALF_DUPLEX_RX_TIMEOUT 0x02
#define HALF_DUPLEX_RX_DONE 0x04
#define HALF_DUPLEX_TX_ERROR 0x08
#define HALF_DUPLEX_TX_DONE 0x10

struct pio_sm
{
	__IOM uint32_t clkdiv;
	__IOM uint32_t execctrl;
	__IOM uint32_t shiftctrl;
	__IOM uint32_t addr;
	__IOM uint32_t instr;
	__IOM uint32_t pinctrl;
};

static uint32_t pio_loaded = 0;

static inline bool is_pio_initialized(unsigned int pio)
{
	return (pio_loaded & (1UL << pio)) != 0;
}

static inline void set_pio_initialized(unsigned int pio)
{
	pio_loaded |= (1UL << pio);
}

static inline void clear_pio_initialized(unsigned int pio)
{
	pio_loaded &= ~(1UL << pio);
}

static void channel_interrupt_handler(uint32_t machine, uint32_t source, void *context)
{
	assert(context != 0);

	struct half_duplex *hd = context;
	if (hd->channel_state != HALF_DUPLEX_RX) {
		++hd->error_ctr;
		return;
	}

	/* Check from framing error */
	uint32_t events;
	if (hal_pio_is_sticky(hd->pio_machine, hd->frame_error_mask))
		events = HALF_DUPLEX_RX_ERROR;
	else
		events = HALF_DUPLEX_RX_TIMEOUT;

	/* Post the event */
	events = osEventFlagsSet(hd->channel_event, events);
	if (events & osFlagsError)
		abort();
}

static void dma_handler(uint32_t channel, void *context)
{
	assert(context != 0);

	uint32_t events;
	struct half_duplex *hd = context;

	/* Disable the DMA, not sure why this is required but... */
	HW_CLEAR_ALIAS(hd->dma->alt1_ctrl) = 0x00000001;

	/* Set flags based on channel state */
	switch (hd->channel_state) {
		case HALF_DUPLEX_RX:
			events = (hd->dma->ctrl_trig & DMA_CH9_CTRL_TRIG_AHB_ERROR_Msk) == 0 ? HALF_DUPLEX_RX_DONE : HALF_DUPLEX_RX_ERROR;
			break;
		case HALF_DUPLEX_TX:
			events = (hd->dma->ctrl_trig & DMA_CH9_CTRL_TRIG_AHB_ERROR_Msk) == 0 ? HALF_DUPLEX_TX_DONE : HALF_DUPLEX_TX_ERROR;
			break;
		case HALF_DUPLEX_IDLE:
		default:
			return;
	}

	/* Post the event */
	events = osEventFlagsSet(hd->channel_event, events);
	if (events & osFlagsError)
		abort();
}

int half_duplex_ini(struct half_duplex *hd, uint32_t pio_channel, uint32_t pin, uint32_t dma_channel)
{
	assert(hd != 0);

	memset(hd, 0, sizeof(struct half_duplex));

	/* Basic initialization */
	hd->pin = pin;
	hd->pio_machine = pio_channel;
	hd->dma_channel = dma_channel;

	/* Load up the hardware interface */
	hd->machine = hal_pio_get_machine(hd->pio_machine);
	hd->rx_fifo = (uint8_t *)hal_pio_get_rx_fifo(hd->pio_machine) + 3;
	hd->tx_fifo = (uint8_t *)hal_pio_get_tx_fifo(hd->pio_machine) + 3;
	hd->rx_timeout_mask = (1UL << (PIO0_INTR_SM0_Pos + (hd->pio_machine & 0x3)));
	hd->frame_error_mask = (1UL << ((hd->pio_machine & 0x3) + 4));

	hd->rx_dreq = hal_pio_get_rx_dreg(hd->pio_machine);
	hd->tx_dreq = hal_pio_get_tx_dreg(hd->pio_machine);

	/* Initialize the busy lock */
	osMutexAttr_t mutex_attr = { .name = "half-duplex-channel", .attr_bits = osMutexPrioInherit };
	hd->channel_lock = osMutexNew(&mutex_attr);
	if (!hd->channel_lock)
		return -errno;

	/* Initialize the completion event */
	osEventFlagsAttr_t event_attr = { .name = "half-duplex-channel" };
	hd->channel_event = osEventFlagsNew(&event_attr);
	if (!hd->channel_event)
		return -errno;

	/* Do we need configure the PIO? */
	unsigned int pio = (hd->pio_machine >> 1);
	if (!is_pio_initialized(pio)) {

		/* Load the half duplex program */
		hal_pio_load_program(hd->pio_machine, half_duplex_program_instructions, array_sizeof(half_duplex_program_instructions), 0);

		/* Mark as initialized */
		set_pio_initialized(pio);
	}

	/* Setup the channel pin control */
	hd->machine->pinctrl = (2UL << PIO0_SM0_PINCTRL_SIDESET_COUNT_Pos) | (1UL << PIO0_SM0_PINCTRL_SET_COUNT_Pos) | (1UL << PIO0_SM0_PINCTRL_OUT_COUNT_Pos) | (hd->pin << PIO0_SM0_PINCTRL_SET_BASE_Pos) | (hd->pin << PIO0_SM0_PINCTRL_OUT_BASE_Pos) | (hd->pin << PIO0_SM0_PINCTRL_SIDESET_BASE_Pos) | (hd->pin << PIO0_SM0_PINCTRL_IN_BASE_Pos);

	/* Setup the channel execution configuration */
	hd->machine->execctrl &= ~(PIO0_SM0_EXECCTRL_WRAP_BOTTOM_Msk | PIO0_SM0_EXECCTRL_WRAP_TOP_Msk);
	hd->machine->execctrl |= (half_duplex_wrap_target << PIO0_SM0_EXECCTRL_WRAP_BOTTOM_Pos) | (half_duplex_wrap << PIO0_SM0_EXECCTRL_WRAP_TOP_Pos) | (1UL << PIO0_SM0_EXECCTRL_STATUS_N_Pos) | PIO0_SM0_EXECCTRL_SIDE_EN_Msk | (13UL << PIO0_SM0_EXECCTRL_JMP_PIN_Pos);

	/* Register our interrupt handler for RX timeouts */
	hal_pio_register_irq(hd->pio_machine, channel_interrupt_handler, hd);
	hal_pio_clear_irq(hd->pio_machine, hd->rx_timeout_mask);
	hal_pio_clear_sticky(hd->pio_machine, hd->frame_error_mask);
	hal_pio_enable_irq(hd->pio_machine, hd->rx_timeout_mask);

	/* Get the DMA hardware interface used for RX and TX and register our interrupt handler */
	hd->dma = hal_dma_get(hd->dma_channel);
	hal_dma_register_irq(hd->dma_channel, dma_handler, hd);
	hal_dma_enable_irq(hd->dma_channel);

	/* Configure, default speed and timeout */
	half_duplex_configure(hd, HALF_DUPLEX_DEFAULT_BAUD_RATE, HALF_DUPLEX_DEFAULT_RX_TIMEOUT);

	/* Ready to go */
	return 0;
}

void half_duplex_fini(struct half_duplex *hd)
{
	assert(hd != 0);

	/* First disable */
	hal_pio_machine_disable(hd->pio_machine);

	/* Now the interrupt, FIXME need correct source mask */
	hal_pio_disable_irq(hd->pio_machine, MACHINE_IRQ_SOURCE(hd->pio_machine, MACHINE_RXNEMPTY_Msk | MACHINE_TNXFULL_Msk | MACHINE_LOCAL_Msk));
	hal_pio_unregister_irq(hd->pio_machine);

	/* And the DMA channel */
	hal_dma_disable_irq(hd->dma_channel);
	hal_dma_unregister_irq(hd->dma_channel);
}

struct half_duplex *half_duplex_create(uint32_t pio_channel, uint32_t pin, uint32_t dma_channel)
{
	/* Need some space */
	struct half_duplex *hd = malloc(sizeof(struct half_duplex));
	if (!hd)
		return 0;

	/* Forward to initializers */
	int status = half_duplex_ini(hd, pio_channel, pin, dma_channel);
	if (status < 0) {
		free(hd);
		return 0;
	}

	return 0;
}

void half_duplex_destroy(struct half_duplex *hd)
{
	assert(hd != 0);

	/* Forward */
	half_duplex_fini(hd);

	/* Done as we can not fail */
	free(hd);
}

int half_duplex_configure(struct half_duplex *hd, uint32_t baud_rate, uint32_t rx_idle_timeout)
{
	assert(hd != 0);

	/* Check the baud rate */
	if (baud_rate > 4000000UL) {
		errno = EINVAL;
		return -errno;
	}

	/* Lock the channel */
	osStatus_t os_status = osMutexAcquire(hd->channel_lock, osWaitForever);
	if (os_status != osOK)
		return -errno;

	/* Up date the channel */
	hd->speed = baud_rate;
	hd->rx_idle_timeout = rx_idle_timeout;

	/* Disable the channel */
	hal_pio_machine_disable(hd->pio_machine);

	/* Load channel baud rate */
	float div = SystemCoreClock / (16.0 * hd->speed);
	uint32_t div_int = div;
	uint32_t div_frac = (div - div_int) * 256;
	hd->machine->clkdiv = (div_int << PIO0_SM0_CLKDIV_INT_Pos) | (div_frac << PIO0_SM0_CLKDIV_FRAC_Pos);

	/* RX timeout in bytes x 16 clocks times, remember 10 bits per byte on the wire, manually load this into the state machine Y register */
	*hal_pio_get_tx_fifo(hd->pio_machine) = hd->rx_idle_timeout * 10 * 16;

	/* Pull from the TX fifo */
	hd->machine->instr = 0x80a0;

	/* Load Y with the the timeout values */
	hd->machine->instr = 0xa047;

	/* Jump to the entry point */
	hd->machine->instr = half_duplex_rx_switch;

	/* Enable the channel */
	hal_pio_machine_enable(hd->pio_machine);

	/* All done with the channel */
	os_status = osMutexRelease(hd->channel_lock);
	if (os_status != 0)
		return -errno;

	/* Should be good */
	return 0;
}

void half_duplex_tx_test(struct half_duplex *hd, const void *buffer, size_t count);
void half_duplex_tx_test(struct half_duplex *hd, const void *buffer, size_t count)
{
	hd->machine->instr = half_duplex_tx_switch;

	for (size_t i = 0; i < count; ++i, ++buffer) {

		/* Wait for space */
		while (hal_pio_tx_full(hd->pio_machine)) {
			osThreadYield();
			board_led_toggle(0);
		}

		/* Push into fifo */
		*hd->tx_fifo = *(const uint8_t *)buffer;
	}
}

void half_duplex_rx_test(struct half_duplex *hd, void *buffer, size_t count);
void half_duplex_rx_test(struct half_duplex *hd, void *buffer, size_t count)
{
	for (size_t i = 0; i < count; ++i, ++buffer) {

		/* Wait for data */
		while (hal_pio_rx_empty(hd->pio_machine)) {
			osThreadYield();
			board_led_toggle(0);
		}

		/* Push into fifo */
		*(uint8_t *)buffer = *hd->rx_fifo;
	}
}

ssize_t half_duplex_send(struct half_duplex *hd, const void *buffer, size_t count)
{
	assert(hd != 0 && buffer != 0);

	ssize_t status = count;

	/* Lock the channel */
	osStatus_t os_status = osMutexAcquire(hd->channel_lock, osWaitForever);
	if (os_status != osOK)
		return -errno;

	/* Mark the state */
	hd->channel_state = HALF_DUPLEX_TX;

	/* Clear the tx events */
	osEventFlagsClear(hd->channel_event, HALF_DUPLEX_TX_DONE | HALF_DUPLEX_TX_ERROR);

	/* Switch to tx */
	hd->machine->instr = half_duplex_tx_switch;

	/* Configure and start the DMA channel */
	hd->dma->read_addr = (uint32_t)buffer;
	hd->dma->write_addr = (uint32_t)hd->tx_fifo;
	hd->dma->trans_count = count;
	hd->dma->ctrl_trig = DMA_CH0_CTRL_TRIG_READ_ERROR_Msk | DMA_CH0_CTRL_TRIG_WRITE_ERROR_Msk | (hd->tx_dreq << DMA_CH0_CTRL_TRIG_TREQ_SEL_Pos) | DMA_CH0_CTRL_TRIG_INCR_READ_Msk | DMA_CH0_CTRL_TRIG_EN_Msk;

	/* Wait for the transfer to complete */
	uint32_t events = osEventFlagsWait(hd->channel_event, HALF_DUPLEX_TX_DONE | HALF_DUPLEX_TX_ERROR, osFlagsWaitAny, osWaitForever);
	if (events & osFlagsError) {
		status = -errno;
		goto error;
	}

	/* Check the a DMA error? */
	if (events & HALF_DUPLEX_TX_ERROR) {
		errno = EIO;
		status = -errno;
	}

	/* Wait for the TX to go idle */
	while (!hal_pio_tx_empty(hd->pio_machine) || hd->machine->addr != half_duplex_tx_idle);

	/* All done for now */
error:
	hd->channel_state = HALF_DUPLEX_IDLE;
	os_status = osMutexRelease(hd->channel_lock);
	if (os_status != 0)
		return -errno;

	return status;
}

ssize_t half_duplex_recv(struct half_duplex *hd, void *buffer, size_t count, unsigned int msecs)
{
	assert(hd != 0 && buffer != 0);

	ssize_t status = count;

	/* Lock the channel */
	osStatus_t os_status = osMutexAcquire(hd->channel_lock, osWaitForever);
	if (os_status != osOK)
		return -errno;

	/* Mark the state */
	hd->channel_state = HALF_DUPLEX_RX;

	/* Clear the Rx events */
	osEventFlagsClear(hd->channel_event, HALF_DUPLEX_RX_DONE | HALF_DUPLEX_RX_ERROR | HALF_DUPLEX_RX_TIMEOUT);

	/* Clear the rx flags */
	hal_pio_clear_irq(hd->pio_machine, hd->rx_timeout_mask);
	hal_pio_clear_sticky(hd->pio_machine, hd->frame_error_mask);

	/* Switch to rx */
	hd->machine->instr = half_duplex_rx_switch;

	/* Configure and start the DMA channel */
	hd->dma->read_addr = (uint32_t)hd->rx_fifo;
	hd->dma->write_addr = (uint32_t)buffer;
	hd->dma->trans_count = count;
	hd->dma->ctrl_trig = DMA_CH0_CTRL_TRIG_READ_ERROR_Msk | DMA_CH0_CTRL_TRIG_WRITE_ERROR_Msk | (hd->rx_dreq << DMA_CH0_CTRL_TRIG_TREQ_SEL_Pos) | DMA_CH0_CTRL_TRIG_INCR_WRITE_Msk | DMA_CH0_CTRL_TRIG_EN_Msk;

	/* Wait for the transfer to complete */
	uint32_t events = osEventFlagsWait(hd->channel_event, HALF_DUPLEX_RX_DONE | HALF_DUPLEX_RX_ERROR | HALF_DUPLEX_RX_TIMEOUT, osFlagsWaitAny, msecs);
	if (events & osFlagsError) {
		hal_dma_abort(hd->dma_channel);
		status = -errno;
		goto error;
	}

	/* Check DMA error or framing error */
	if (events & HALF_DUPLEX_RX_ERROR) {
		hal_dma_abort(hd->dma_channel);
		errno = EIO;
		status = -errno;
		goto error;
	}

	/* Short RX? */
	if (events & HALF_DUPLEX_RX_TIMEOUT) {
		status = count - hd->dma->trans_count;
		if (status < count)
			hal_dma_abort(hd->dma_channel);
	}

	/* All done for now */
error:
	hd->channel_state = HALF_DUPLEX_IDLE;
	os_status = osMutexRelease(hd->channel_lock);
	if (os_status != 0)
		return -errno;

	/* status contains amount read or error */
	return status;
}



