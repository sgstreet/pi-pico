/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  Copyright: @2024 Stephen Street (stephen@redrocketcomputing.com)
 */

#include <config.h>

#include <cmsis/cmsis.h>

#include <init/init-sections.h>
#include <sys/irq.h>
#include <sys/syslog.h>

#include <hardware/rp2040/exti.h>

struct exti_entry
{
	uint32_t trigger;
	exti_handler_t handler;
	void *context;
};

static __isr_section __optimize void exti_default_handler(EXTIn_Type exti, uint32_t state, void *context)
{
}

struct exti_entry exti_dispatch[EXTI_NUM] = { [0 ... EXTI_NUM - 1] = { .handler = exti_default_handler, .context = 0 } };

void IO_IRQ_BANK0_Handler(void);
void IO_IRQ_BANK0_Handler(void)
{
	volatile uint32_t *status_reg = SystemCurrentCore == 0 ? &IO_BANK0->PROC0_INTS0 : &IO_BANK0->PROC1_INTS0;
	volatile uint32_t *clear_reg = &IO_BANK0->INTR0;
	for (size_t i = 0; i < 30; ++i) {
		uint32_t indx = i >> 3;
		uint32_t shift = (i & 7) << 2;
		uint32_t state = (mask_reg(clear_reg + indx, 0xf << shift) >> shift) & (EXTI_LEVEL_LOW | EXTI_LEVEL_HIGH);
		if (mask_reg(status_reg + indx, 0xf << shift) != 0) {
			exti_dispatch[i].handler(i | (SystemCurrentCore << 31), state, exti_dispatch[i].context);
			clear_mask(clear_reg + indx, 0xf << shift);
		}
	}
}

void exti_register(EXTIn_Type exti, EXTI_Trigger trigger, uint32_t priority, exti_handler_t handler, void *context)
{
	exti_set_context(exti, context);
	exti_set_handler(exti, handler);
	exti_set_trigger(exti, trigger);
}

void exti_unregister(EXTIn_Type exti, exti_handler_t handler)
{
	exti_disable(exti);
	exti_set_handler(exti, exti_default_handler);
	exti_set_trigger(exti, 0);
	exti_set_context(exti, 0);
}

EXTI_Trigger exti_get_trigger(EXTIn_Type exti)
{
	size_t idx = exti & 0x7fffffff;
	return exti_dispatch[idx].trigger;
}

void exti_set_trigger(EXTIn_Type exti, EXTI_Trigger trigger)
{
	size_t idx = exti & 0x7fffffff;
	exti_dispatch[idx].trigger = trigger;
}

exti_handler_t exti_get_handler(EXTIn_Type exti)
{
	size_t idx = exti & 0x7fffffff;
	return exti_dispatch[idx].handler;
}

void exti_set_handler(EXTIn_Type exti, exti_handler_t handler)
{
	size_t idx = exti & 0x7fffffff;
	exti_dispatch[idx].handler = handler;
}

void *exti_get_context(EXTIn_Type exti)
{
	size_t idx = exti & 0x7fffffff;
	return exti_dispatch[idx].context;
}

void exti_set_context(EXTIn_Type exti, void *context)
{
	size_t idx = exti & 0x7fffffff;
	exti_dispatch[idx].context = context;
}

void exti_set_priority(EXTIn_Type exti, uint32_t priority)
{
	/* Ignored */
}

uint32_t exti_get_priority(EXTIn_Type exti)
{
	return irq_get_priority(IO_IRQ_BANK0_IRQn);
}

void exti_enable(EXTIn_Type exti)
{
	exti_clear(exti);

	volatile uint32_t *core = (exti & 0x80000000) == 0 ? &IO_BANK0->PROC0_INTE0 : &IO_BANK0->PROC1_INTE0;
	uint32_t actual = exti & 0x7fffffff;
	uint32_t indx = actual >> 3;
	uint32_t shift = (actual & 0x7) << 2;
	set_mask(core + indx, exti_dispatch[actual].trigger << shift);
}

void exti_disable(EXTIn_Type exti)
{
	volatile uint32_t *core = (exti & 0x80000000) == 0 ? &IO_BANK0->PROC0_INTE0 : &IO_BANK0->PROC1_INTE0;
	uint32_t actual = exti & 0x7fffffff;
	uint32_t indx = actual >> 3;
	uint32_t shift = (actual & 0x7) << 2;
	clear_mask(core + indx, 0xf << shift);
}

bool exti_is_enabled(EXTIn_Type exti)
{
	volatile uint32_t *core = (exti & 0x80000000) == 0 ? &IO_BANK0->PROC0_INTE0 : &IO_BANK0->PROC1_INTE0;
	uint32_t actual = exti & 0x7fffffff;
	uint32_t indx = actual >> 3;
	uint32_t shift = (actual & 7) << 2;
	return (core[indx] & (0xf << shift)) != 0;
}

void exti_trigger(EXTIn_Type exti)
{
	syslog_fatal("not implemented\n");
}

bool exti_is_pending(EXTIn_Type exti)
{
	volatile uint32_t *status_reg = SystemCurrentCore == 0 ? &IO_BANK0->PROC0_INTS0 : &IO_BANK0->PROC1_INTS0;
	uint32_t actual = exti & 0x7fffffff;
	uint32_t indx = actual >> 3;
	uint32_t shift = (actual & 0x7) << 2;
	return mask_reg(status_reg + indx, exti_dispatch[actual].trigger << shift) != 0;
}

void exti_clear(EXTIn_Type exti)
{
	volatile uint32_t *reg = &IO_BANK0->INTR0;
	uint32_t actual = exti & 0x7fffffff;
	uint32_t indx = actual >> 3;
	uint32_t shift = (actual & 7) << 2;
	clear_mask(reg + indx, 0xf << shift);
}

void exti_init(void)
{
	/* Clear all raw interrupts */
	for (EXTIn_Type exti = EXTIn_CORE0_0; exti <= EXTIn_CORE0_29; ++exti)
		exti_clear(exti);

	/* Enable the IO bank interrupt */
	irq_set_priority(IO_IRQ_BANK0_IRQn, INTERRUPT_NORMAL);
	irq_enable(IO_IRQ_BANK0_IRQn);
}
PREINIT_SYSINIT_WITH_PRIORITY(exti_init, EXTI_SYSTEM_INIT_PRIORIY);
