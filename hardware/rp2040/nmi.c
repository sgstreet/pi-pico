/*
 * Copyright (C) 2024 Stephen Street
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * pico-nmi.c
 *
 *  Created on: Apr 10, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <stdatomic.h>

#include <sys/tls.h>
#include <sys/irq.h>

#include <cmsis/cmsis.h>
#include <hardware/rp2040/nmi.h>

#define NUM_NVIC_IRQ (__LAST_IRQN + 1)
#define nmi_proc_mask (*(volatile uint32_t *)(&SYSCFG->PROC0_NMI_MASK + SystemCurrentCore))

typedef void (*raw_irq_handler_t)(void);

void NMI_Handler(void);

static core_local raw_irq_handler_t active_handlers[NUM_NVIC_IRQ + 1] = { 0 };

__optimize void NMI_Handler(void)
{
	/* Dispatch all boosted nmi handlers */
	raw_irq_handler_t *pos = cls_datum(active_handlers);
	raw_irq_handler_t current;
	while ((current = *pos++) != 0)
		current();
}

void nmi_set_enable(IRQn_Type irq, bool enabled)
{
	assert(irq >= 0 && irq < NUM_NVIC_IRQ);

	/* Skip if we are already in the correct state */
	if (nmi_is_enabled(irq) == enabled)
		return;

	/* Project the active handlers table */
	uint32_t state = disable_interrupts();
	uint32_t nmi_state = nmi_proc_mask;
	nmi_proc_mask = 0;

	if (enabled) {

		/* Look for the end of the table place the handler there */
		for (size_t idx = 0; idx < NUM_NVIC_IRQ; ++idx)
			if (cls_datum(active_handlers)[idx] == 0) {

				/* Add the handler to the table */
				cls_datum(active_handlers)[idx] = ((raw_irq_handler_t *)SCB->VTOR)[irq + 16];

				/* And to the hardware mask */
				nmi_state |= (1UL << irq);
				break;
			}

	} else {

		/* Remove and compress table in one pass */
		for (size_t idx = 0, found = false; idx < NUM_NVIC_IRQ && cls_datum(active_handlers)[idx] != 0; ++idx)
			if (found || (found = (cls_datum(active_handlers)[idx] == ((raw_irq_handler_t *)SCB->VTOR)[irq + 16])))
				cls_datum(active_handlers)[idx] = cls_datum(active_handlers)[idx + 1];

		/* Remove interrupt from the IRQ mask */
		nmi_state &= ~(1UL << irq);
	}

	/* Let it fly */
	nmi_proc_mask = nmi_state;
	enable_interrupts(state);
}

bool nmi_is_enabled(IRQn_Type irq)
{
	return (nmi_proc_mask & (1UL << irq)) != 0;
}

uint64_t nmi_mask(void)
{
	return atomic_exchange((uint64_t *)&SYSCFG->PROC0_NMI_MASK, 0);
}

void nmi_unmask(uint64_t state)
{
	atomic_exchange((uint64_t *)&SYSCFG->PROC0_NMI_MASK, state);
}
