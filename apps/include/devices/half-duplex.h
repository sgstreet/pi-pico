/*
 * half-duplex.h
 *
 *  Created on: Jan 2, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _HALF_DUPLEX_H_
#define _HALF_DUPLEX_H_

#include <stdatomic.h>
#include <stdbool.h>
#include <sys/types.h>

#include <rtos/rtos.h>
#include <hal/hal.h>

#define HALF_DUPLEX_DEFAULT_BAUD_RATE 115200UL
#define HALF_DUPLEX_DEFAULT_RX_TIMEOUT 4

enum half_duplex_state
{
	HALF_DUPLEX_IDLE = 0,
	HALF_DUPLEX_RX,
	HALF_DUPLEX_TX,
};

struct half_duplex
{
	uint32_t pin;
	uint32_t speed;
	uint32_t rx_idle_timeout;
	uint32_t pio_machine;
	uint32_t dma_channel;

	uint32_t tx_dreq;
	uint32_t rx_dreq;

	uint32_t rx_timeout_mask;
	uint32_t frame_error_mask;

	struct hal_pio_machine *machine;
	volatile uint8_t *rx_fifo;
	volatile uint8_t *tx_fifo;

	struct hal_dma_channel *dma;

	osMutexId_t channel_lock;
	osEventFlagsId_t channel_event;

	enum half_duplex_state channel_state;
	unsigned int error_ctr;
};

struct half_duplex_config
{
	uint32_t channel;
	uint32_t pin;
	uint32_t dma_channel;
};

int half_duplex_ini(struct half_duplex *hd, uint32_t pio_channel, uint32_t pin, uint32_t dma_channel);
void half_duplex_fini(struct half_duplex *hd);

struct half_duplex *half_duplex_create(uint32_t pio_channel, uint32_t pin, uint32_t dma_channel);
void half_duplex_destroy(struct half_duplex *hd);

int half_duplex_configure(struct half_duplex *hd, uint32_t baud_rate, uint32_t rx_idle_timeout);

ssize_t half_duplex_send(struct half_duplex *hd, const void *buffer, size_t count);
ssize_t half_duplex_recv(struct half_duplex *hd, void *buffer, size_t count, unsigned int msecs);

#endif
