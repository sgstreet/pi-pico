/*
 * hal-dma.c
 *
 *  Created on: Jan 3, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>

#include <compiler.h>

#include <init/init-sections.h>

#include <hal/rp2040/hal-rp2040.h>

struct hal_dma_channel_handler
{
	hal_dma_interrupt_handler_t handler;
	void *context;
};

static void hal_dma_default_irq_handler(uint32_t channel, void *context);

static struct hal_dma_channel_handler core0_channel_handlers[] = { [0 ... HAL_NUM_DMA_CHANNELS - 1] = { .handler = hal_dma_default_irq_handler, .context = 0 } };
static struct hal_dma_channel_handler core1_channel_handlers[] = { [0 ... HAL_NUM_DMA_CHANNELS - 1] = { .handler = hal_dma_default_irq_handler, .context = 0 } };

void DMA_IRQ_0_Handler(void);
__isr_section void DMA_IRQ_0_Handler(void)
{
	uint32_t ints = DMA->INTS0;
	DMA->INTS0 = ints;
	for (uint32_t i = 0; i < HAL_NUM_DMA_CHANNELS && ints != 0; ++i, ints >>= 1)
		if (ints & 0x00000001)
			core0_channel_handlers[i].handler(i, core0_channel_handlers[i].context);
}

void DMA_IRQ_1_Handler(void);
__isr_section void DMA_IRQ_1_Handler(void)
{
	uint32_t ints = DMA->INTS1;
	DMA->INTS1 = ints;
	for (uint32_t i = 0; i < HAL_NUM_DMA_CHANNELS && ints != 0; ++i, ints >>= 1)
		if (ints & 0x00000001)
			core1_channel_handlers[i].handler(i, core0_channel_handlers[i].context);
}

static void hal_dma_default_irq_handler(uint32_t channel, void *context)
{
}

struct hal_dma_channel* hal_dma_get(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	return ((void *)&DMA->CH0_READ_ADDR) + channel * sizeof(struct hal_dma_channel);
}

void hal_dma_register_irq(uint32_t channel, hal_dma_interrupt_handler_t handler, void *context)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	if (SIO->CPUID) {
		core1_channel_handlers[channel].context = context;
		core1_channel_handlers[channel].handler = handler;
	} else {
		core0_channel_handlers[channel].context = context;
		core0_channel_handlers[channel].handler = handler;
	}
}

void hal_dma_unregister_irq(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	if (SIO->CPUID) {
		core1_channel_handlers[channel].handler = hal_dma_default_irq_handler;
		core1_channel_handlers[channel].context = 0;
	} else {
		core0_channel_handlers[channel].handler = hal_dma_default_irq_handler;
		core0_channel_handlers[channel].context = 0;
	}
}

void hal_dma_enable_irq(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	if (SIO->CPUID)
		HW_SET_ALIAS(DMA->INTE1) = (1UL << channel);
	else
		HW_SET_ALIAS(DMA->INTE0) = (1UL << channel);
}

void hal_dma_disable_irq(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	if (SIO->CPUID)
		HW_CLEAR_ALIAS(DMA->INTE1) = (1UL << channel);
	else
		HW_CLEAR_ALIAS(DMA->INTE0) = (1UL << channel);
}

void hal_dma_clear_irq(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	if (SIO->CPUID)
		DMA->INTS1 = (1UL << channel);
	else
		DMA->INTS0 = (1UL << channel);
}

void hal_dma_start(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);
	HW_SET_ALIAS(DMA->MULTI_CHAN_TRIGGER) = (1UL << channel);
}

void hal_dma_abort(uint32_t channel)
{
	assert(channel < HAL_NUM_DMA_CHANNELS);

	struct hal_dma_channel *dma_channel = hal_dma_get(channel);
	hal_dma_disable_irq(channel);
	DMA->CHAN_ABORT = (1UL << channel);
	while (dma_channel->ctrl_trig & DMA_CH0_CTRL_TRIG_BUSY_Msk);
	hal_dma_clear_irq(channel);
	hal_dma_enable_irq(channel);
}

static void hal_dma_init(void)
{
	if (SIO->CPUID)
		NVIC_EnableIRQ(DMA_IRQ_1_IRQn);
	else
		NVIC_EnableIRQ(DMA_IRQ_0_IRQn);
}
PREINIT_PLATFORM_WITH_PRIORITY(hal_dma_init, HAL_PLATFORM_PRIORITY);
