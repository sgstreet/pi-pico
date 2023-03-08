/*
 * hal-rp2040-dma.h
 *
 *  Created on: Jan 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _HAL_RP2040_DMA_H_
#define _RP2040_DMA_H_

#define HAL_NUM_DMA_CHANNELS 12UL

typedef void (*hal_dma_interrupt_handler_t)(uint32_t channel, void *context);

struct hal_dma_channel
{
	__IOM uint32_t read_addr;
	__IOM uint32_t write_addr;
	__IOM uint32_t trans_count;
	__IOM uint32_t ctrl_trig;
	__IOM uint32_t alt1_ctrl;
	__IOM uint32_t alt1_read_addr;
	__IOM uint32_t alt1_write_addr;
	__IOM uint32_t alt1_trans_count_trig;
	__IOM uint32_t alt2_ctrl;
	__IOM uint32_t alt2_trans_count;
	__IOM uint32_t alt2_read_addr;
	__IOM uint32_t alt2_write_addr_trig;
	__IOM uint32_t alt3_ctrl;
	__IOM uint32_t alt3_write_addr;
	__IOM uint32_t alt3_trans_count;
	__IOM uint32_t alt3_read_addr_trig;
};

struct hal_dma_channel* hal_dma_get(uint32_t channel);

void hal_dma_register_irq(uint32_t channel, hal_dma_interrupt_handler_t handler, void *context);
void hal_dma_unregister_irq(uint32_t channel);

void hal_dma_enable_irq(uint32_t channel);
void hal_dma_disable_irq(uint32_t channel);
void hal_dma_clear_irq(uint32_t channel);

void hal_dma_start(uint32_t channel);
void hal_dma_abort(uint32_t channel);

#endif
