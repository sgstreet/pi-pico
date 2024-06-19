/*
 * hal-dma.c
 *
 *  Created on: Jan 3, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <stdlib.h>

#include <compiler.h>

#include <sys/irq.h>

#include <init/init-sections.h>

#include <hal/rp2040/hal-rp2040.h>

// TODO Build macro expansions for channel alt register addresses
//#define CHANNAL_REG_ADDR(CHANNEL) ((volatile uint32_t *)((void *)DMA->CH0_CTRL_TRIG + (CHANNEL << 6UL)))

struct hal_dma_channel_handler
{
	hal_dma_channel_handler_t handler;
	void *context;
	IRQn_Type irq;

};

static void hal_dma_default_channel_handler(uint32_t channel, void *context);

static struct hal_dma_channel_handler channel_handlers[] = { [0 ... HAL_NUM_DMA_CHANNELS - 1] = { .handler = hal_dma_default_channel_handler, .context = 0, .irq = DMA_IRQ_0_IRQn } };

static void hal_dma_default_channel_handler(uint32_t channel, void *context)
{
}

static __isr_section void dma_irq_handler(IRQn_Type irq, void *context)
{
	volatile uint32_t *ints_addr = context;

	/* Read and clear the interrupts */
	uint32_t ints = *ints_addr;
	*ints_addr = ints;

	/* Dispatch to the matching channel handlers */
	for (uint32_t i = 0; i < HAL_NUM_DMA_CHANNELS && ints != 0; ++i, ints >>= 1)
		if (ints & 0x00000001)
			channel_handlers[i].handler(i, channel_handlers[i].context);
}

void hal_dma_register_channel(uint32_t channel, IRQn_Type irq, hal_dma_channel_handler_t handler, void *context)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	channel_handlers[channel].irq = irq;
	channel_handlers[channel].context = context;
	channel_handlers[channel].handler = handler;
}

void hal_dma_unregister_channel(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	channel_handlers[channel].handler = hal_dma_default_channel_handler;
	channel_handlers[channel].context = 0;
	channel_handlers[channel].irq = DMA_IRQ_0_IRQn;
}

void hal_dma_enable_irq(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	if (channel_handlers[channel].irq == DMA_IRQ_0_IRQn)
		set_bit(&DMA->INTE0, channel);
	else
		set_bit(&DMA->INTE1, channel);
}

void hal_dma_disable_irq(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	if (channel_handlers[channel].irq == DMA_IRQ_0_IRQn)
		clear_bit(&DMA->INTE0, channel);
	else
		clear_bit(&DMA->INTE1, channel);
}

void hal_dma_clear_irq(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	if (channel_handlers[channel].irq == DMA_IRQ_0_IRQn)
		set_bit(&DMA->INTS0, channel);
	else
		set_bit(&DMA->INTS1, channel);
}

void hal_dma_clear_ctrl(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *ctrl = (void *)&DMA->CH0_AL1_CTRL + (channel << 6UL);
	*ctrl = DMA_CH0_CTRL_TRIG_AHB_ERROR_Msk | DMA_CH0_CTRL_TRIG_READ_ERROR_Msk | DMA_CH0_CTRL_TRIG_WRITE_ERROR_Msk | (channel << DMA_CH0_CTRL_TRIG_CHAIN_TO_Pos);
}

void hal_dma_set_read_addr(uint32_t channel, uintptr_t addr)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *read_addr = (void *)&DMA->CH0_READ_ADDR + (channel << 6UL);
	*read_addr = addr;
}

void hal_dma_set_write_addr(uint32_t channel, uintptr_t addr)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *write_addr = (void *)&DMA->CH0_WRITE_ADDR + (channel << 6UL);
	*write_addr = addr;
}

void hal_dma_set_transfer_count(uint32_t channel, uint32_t count)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *count_addr = (void *)&DMA->CH0_TRANS_COUNT + (channel << 6UL);
	*count_addr = count;
}

void hal_dma_set_sniff(uint32_t channel, bool enabled)
{

}
void hal_dma_set_irq_quiet(uint32_t channel, bool enabled)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *ctrl = (void *)&DMA->CH0_AL1_CTRL + (channel << 6UL);

	if (enabled)
		set_bit(ctrl, DMA_CH0_CTRL_TRIG_SNIFF_EN_Pos);
	else
		clear_bit(ctrl, DMA_CH0_CTRL_TRIG_SNIFF_EN_Pos);
}

void hal_dma_set_treq_sel(uint32_t channel, enum hal_dma_dreq dreq)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *ctrl = (void *)&DMA->CH0_AL1_CTRL + (channel << 6UL);

	clear_mask(ctrl, DMA_CH0_CTRL_TRIG_TREQ_SEL_Msk);
	set_mask(ctrl, dreq << DMA_CH0_CTRL_TRIG_TREQ_SEL_Pos);
}

void hal_dma_set_chain_channel(uint32_t channel, uint32_t chain_channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS && chain_channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *ctrl = (void *)&DMA->CH0_AL1_CTRL + (channel << 6UL);

	clear_mask(ctrl, DMA_CH0_CTRL_TRIG_CHAIN_TO_Msk);
	set_mask(ctrl, chain_channel << DMA_CH0_CTRL_TRIG_CHAIN_TO_Pos);
}

void hal_dma_set_ring(uint32_t channel, bool selection, uint32_t size)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *ctrl = (void *)&DMA->CH0_AL1_CTRL + (channel << 6UL);

	if (selection)
		set_bit(ctrl, DMA_CH0_CTRL_TRIG_RING_SEL_Pos);
	else
		clear_bit(ctrl, DMA_CH0_CTRL_TRIG_RING_SEL_Pos);

	clear_mask(ctrl, DMA_CH0_CTRL_TRIG_RING_SIZE_Msk);
	set_mask(ctrl, size << DMA_CH0_CTRL_TRIG_RING_SEL_Pos);
}

void hal_dma_set_write_increment(uint32_t channel, bool enabled)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *ctrl = (void *)&DMA->CH0_AL1_CTRL + (channel << 6UL);

	if (enabled)
		set_bit(ctrl, DMA_CH0_CTRL_TRIG_INCR_WRITE_Pos);
	else
		clear_bit(ctrl, DMA_CH0_CTRL_TRIG_INCR_WRITE_Pos);
}

void hal_dma_set_read_increment(uint32_t channel, bool enabled)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *ctrl = (void *)&DMA->CH0_AL1_CTRL + (channel << 6UL);

	if (enabled)
		set_bit(ctrl, DMA_CH0_CTRL_TRIG_INCR_READ_Pos);
	else
		clear_bit(ctrl, DMA_CH0_CTRL_TRIG_INCR_READ_Pos);
}

void hal_dma_set_data_size(uint32_t channel, uint32_t size)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *ctrl = (void *)&DMA->CH0_AL1_CTRL + (channel << 6UL);

	clear_mask(ctrl, DMA_CH0_CTRL_TRIG_DATA_SIZE_Msk);
	set_mask(ctrl, (size >> 1) << DMA_CH0_CTRL_TRIG_DATA_SIZE_Pos);
}

void hal_dma_set_high_priority(uint32_t channel, bool enabled)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *ctrl = (void *)&DMA->CH0_AL1_CTRL + (channel << 6UL);

	if (enabled)
		set_bit(ctrl, DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_Pos);
	else
		clear_bit(ctrl, DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_Pos);
}

void hal_dma_set_enabled(uint32_t channel, bool enabled)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *ctrl = (void *)&DMA->CH0_AL1_CTRL + (channel << 6UL);
	if (enabled)
		set_bit(ctrl, DMA_CH0_CTRL_TRIG_EN_Pos);
	else
		clear_bit(ctrl, DMA_CH0_CTRL_TRIG_EN_Pos);

}

void hal_dma_start(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	set_bit(&DMA->MULTI_CHAN_TRIGGER, channel);
}

bool hal_dma_is_busy(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *ctrl = (void *)&DMA->CH0_AL1_CTRL + (channel << 6UL);

	return (*ctrl & DMA_CH0_CTRL_TRIG_BUSY_Msk) != 0;
}

bool hal_dma_has_error(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *ctrl = (void *)&DMA->CH0_AL1_CTRL + (channel << 6UL);
	return mask_reg(ctrl, DMA_CH0_CTRL_TRIG_AHB_ERROR_Msk) != 0;
}

uint32_t hal_dma_remaining(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	volatile uint32_t *trans_count = (void *)&DMA->CH0_TRANS_COUNT + (channel << 6UL);
	return *trans_count;
}

void hal_dma_abort(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	/* No interrupts while aborting the channel */
	hal_dma_disable_irq(channel);

	/* Request the abort */
	set_mask(&DMA->CHAN_ABORT, (1UL << channel));

	/* Monitor the ctrl reqister for abort completion is is errata RP2040-E13 */
	while (hal_dma_is_busy(channel));

	/* Clear any pended interrupts */
	hal_dma_clear_irq(channel);

	/* And re-enable */
	hal_dma_enable_irq(channel);
}

static void hal_dma_init(void)
{
	/* Clear all the channel controls */
	for (size_t i = 0; i < HAL_NUM_DMA_CHANNELS; ++i)
		hal_dma_clear_ctrl(i);

	/* Ensure all channel interrupts are disabled */
	clear_mask(&DMA->INTE0, DMA_INTE0_INTE0_Msk);
	clear_mask(&DMA->INTE1, DMA_INTE1_INTE1_Msk);

	/* Clear any pend channel interrupts */
	set_mask(&DMA->INTS0, DMA_INTS0_INTS0_Msk);
	set_mask(&DMA->INTS1, DMA_INTS1_INTS1_Msk);

	/* Register irq handler */
	irq_register(DMA_IRQ_0_IRQn, INTERRUPT_NORMAL, dma_irq_handler, (void *)&DMA->INTS0);
	irq_register(DMA_IRQ_1_IRQn, INTERRUPT_NORMAL, dma_irq_handler, (void *)&DMA->INTS1);

	/* Then enable the interrutps */
	irq_enable(DMA_IRQ_0_IRQn);
	irq_enable(DMA_IRQ_1_IRQn);
}
PREINIT_PLATFORM_WITH_PRIORITY(hal_dma_init, HAL_PLATFORM_INIT_PRIORITY);
