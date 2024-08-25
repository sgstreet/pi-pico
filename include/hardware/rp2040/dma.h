/*
 * dma.h
 *
 *  Created on: Jan 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _DMA_H_
#define _DMA_H_

#define DMA_NUM_CHANNELS 12UL

typedef void (*dma_channel_handler_t)(uint32_t channel, void *context);

//struct dma_channel
//{
//	__IOM uint32_t read_addr;
//	__IOM uint32_t write_addr;
//	__IOM uint32_t trans_count;
//	__IOM uint32_t ctrl_trig;
//	__IOM uint32_t alt1_ctrl;
//	__IOM uint32_t alt1_read_addr;
//	__IOM uint32_t alt1_write_addr;
//	__IOM uint32_t alt1_trans_count_trig;
//	__IOM uint32_t alt2_ctrl;
//	__IOM uint32_t alt2_trans_count;
//	__IOM uint32_t alt2_read_addr;
//	__IOM uint32_t alt2_write_addr_trig;
//	__IOM uint32_t alt3_ctrl;
//	__IOM uint32_t alt3_write_addr;
//	__IOM uint32_t alt3_trans_count;
//	__IOM uint32_t alt3_read_addr_trig;
//};

enum dma_dreq
{
	DREQ_PIO0_TX0 = 0,
	DREQ_PIO0_TX1 = 1,
	DREQ_PIO0_TX2 = 2,
	DREQ_PIO0_TX3 = 3,
	DREQ_PIO0_RX0 = 4,
	DREQ_PIO0_RX1 = 5,
	DREQ_PIO0_RX2 = 6,
	DREQ_PIO0_RX3 = 7,

	DREQ_PIO1_TX0 = 8,
	DREQ_PIO1_TX1 = 9,
	DREQ_PIO1_TX2 = 10,
	DREQ_PIO1_TX3 = 11,
	DREQ_PIO1_RX0 = 12,
	DREQ_PIO1_RX1 = 13,
	DREQ_PIO1_RX2 = 14,
	DREQ_PIO1_RX3 = 15,

	DREQ_SPI0_TX = 16,
	DREQ_SPI0_RX = 17,

	DREQ_SPI1_TX = 18,
	DREQ_SPI1_RX = 19,

	DREQ_UART0_TX = 20,
	DREQ_UART0_RX = 21,

	DREQ_UART1_TX = 22,
	DREQ_UART1_RX = 23,

	DREQ_PWM_WRAP0 = 24,
	DREQ_PWM_WRAP1 = 25,
	DREQ_PWM_WRAP2 = 26,
	DREQ_PWM_WRAP3 = 27,
	DREQ_PWM_WRAP4 = 28,
	DREQ_PWM_WRAP5 = 29,
	DREQ_PWM_WRAP6 = 30,
	DREQ_PWM_WRAP7 = 31,

	DREQ_I2C0_TX = 32,
	DREQ_I2C0_RX = 33,

	DREQ_I2C1_TX = 34,
	DREQ_I2C1_RX = 35,

	DREQ_ADC = 36,

	DREQ_XIP_STREAM = 37,
	DREQ_XIP_SSITX = 38,
	DREQ_XIP_SSIRX = 39,
};

enum dma_trigger
{
	CTRL_TRIGGER = 0x00,
	TRANS_COUNT_TRIGGER = 0x10,
	WRITE_ADDR_TRIGGER = 0x20,
	READ_ADDR_TRIGGER = 0x30,
	NO_TRIGGER = 0xff,
};

void dma_register_channel(uint32_t channel, IRQn_Type irq, dma_channel_handler_t handler, void *context);
void dma_unregister_channel(uint32_t channel);

void dma_enable_irq(uint32_t channel);
void dma_disable_irq(uint32_t channel);
void dma_clear_irq(uint32_t channel);

volatile uint32_t *dma_get_csr(uint32_t channel, enum dma_trigger trigger);

void dma_clear_ctrl(uint32_t channel);
void dma_set_read_addr(uint32_t channel, uintptr_t addr);
void dma_set_write_addr(uint32_t channel, uintptr_t addr);
void dma_set_transfer_count(uint32_t channel, uint32_t count);
void dma_set_sniff(uint32_t channel, bool enabled);
void dma_set_irq_quiet(uint32_t channel, bool enabled);
void dma_set_treq_sel(uint32_t channel, enum dma_dreq dreq);
void dma_set_chain_channel(uint32_t channel, uint32_t chain_channel);
void dma_set_ring(uint32_t channel, bool selection, uint32_t size);
void dma_set_write_increment(uint32_t channel, bool enabled);
void dma_set_read_increment(uint32_t channel, bool enabled);
void dma_set_data_size(uint32_t channel, uint32_t size);
void dma_set_high_priority(uint32_t channel, bool enabled);
void dma_set_enabled(uint32_t channel, bool enabled);

void dma_start(uint32_t channel);
void dma_abort(uint32_t channel);

bool dma_is_busy(uint32_t channel);
bool dma_has_error(uint32_t channel);
uint32_t dma_remaining(uint32_t channel);
uint32_t dma_amount(uint32_t channel);

#endif
