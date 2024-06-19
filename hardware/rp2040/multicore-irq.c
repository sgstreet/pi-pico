/*
 * Copyright (C) 2024 Stephen Street
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * irq.c
 *
 *  Created on: Mar 26, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/tls.h>
#include <sys/irq.h>

#include <init/init-sections.h>
#include <cmsis/cmsis.h>

#include <hardware/rp2040/nmi.h>
#include <hardware/rp2040/multicore-event.h>

#define NUM_NVIC_IRQ (__LAST_IRQN + 1)
#define IRQ_EVENT 0x30000000
#define IRQ_COMMAND_MSK 0xff000000

enum irq_cmd
{
	PEND_IRQ_CMD = 0x30000000,
	CLEAR_IRQ_CMD = 0x31000000,
	IRQ_ENABLE_CMD = 0x32000000,
	IRQ_DISABLE_CMD = 0x33000000,
	SET_PRIORITY_CMD = 0x34000000,
};

static core_local bool irq_enabled[NUM_NVIC_IRQ]  = { 0 };
static core_local uint8_t irq_priority[IRQ_NUM]  = { -3, -2, -1, [3 ... IRQ_NUM - 1] = 0 };
static uint8_t irq_affinity[NUM_NVIC_IRQ] = { 0 };

static void multicore_irq_pend_cmd(IRQn_Type irq)
{
	/* These interrupts are in the interrupt control and status register */
	switch (irq) {

		case NonMaskableInt_IRQn:
			SCB->ICSR = SCB_ICSR_NMIPENDSET_Msk;
			break;

		case PendSV_IRQn:
			SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
			break;

		case SysTick_IRQn:
			SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
			break;

		default:
			NVIC_SetPendingIRQ(irq);
			break;
	}
}

static void multicore_irq_clear_cmd(IRQn_Type irq)
{
	/* NVIC interrupt */
	NVIC_ClearPendingIRQ(irq);
	__DMB();
}

static void multicore_irq_enable_cmd(IRQn_Type irq)
{
	/* Drop if system interrupt */
	if (irq < 0)
		return;

	/* Enable, passing real time (-1) priorities to the NMI */
	if (cls_datum(irq_priority)[irq + 16] != UINT8_MAX) {

		/* Clear any pending interrupt */
		multicore_irq_clear_cmd(irq);

		/* Then enable it */
		NVIC_EnableIRQ(irq);

	} else
		nmi_set_enable(irq, true);

	/* And update the cache */
	cls_datum(irq_enabled)[irq] = true;
}

static void multicore_irq_disable_cmd(IRQn_Type irq)
{
	/* Drop if system interrupt */
	if (irq < 0)
		return;

	/* Disables passing real time (-1) priorites to the NMI */
	if (cls_datum(irq_priority)[irq + 16] != UINT8_MAX)
		NVIC_DisableIRQ(irq);
	else
		nmi_set_enable(irq, false);

	/* And Update the cache */
	cls_datum(irq_enabled)[irq] = false;
}

static void multicore_irq_set_priority_cmd(IRQn_Type irq, uint8_t priority)
{
	/* Use 0xff (-1) as the real time interrupt priority i.e. boost to NMI, Use cmsis to correctly handle system interrupts */
	if (priority != UINT8_MAX)
		NVIC_SetPriority(irq, priority);

	/* And Update the cache */
	cls_datum(irq_priority)[irq + 16] = priority;
}

static void multicore_irq_event_handler(uint32_t event, void *context)
{
	/* Handle the command */
	switch (event & IRQ_COMMAND_MSK) {

		case PEND_IRQ_CMD: {
			IRQn_Type irq = (event & 0xff) - 16;
			multicore_irq_pend_cmd(irq);
			break;
		}

		case CLEAR_IRQ_CMD: {
			IRQn_Type irq = (event & 0xff) - 16;
			multicore_irq_clear_cmd(irq);
			break;
		}

		case IRQ_ENABLE_CMD: {
			IRQn_Type irq = (event & 0xff) - 16;
			multicore_irq_enable_cmd(irq);
			break;
		}

		case IRQ_DISABLE_CMD: {
			IRQn_Type irq = (event & 0xff) - 16;
			multicore_irq_disable_cmd(irq);
			break;
		}

		case SET_PRIORITY_CMD: {
			IRQn_Type irq = (event & 0xff) - 16;
			uint8_t priority = (event >> 8) & 0xff;
			multicore_irq_set_priority_cmd(irq, priority);
			break;
		}

		default:
			abort();
	}
}

void irq_set_affinity(IRQn_Type irq, uint32_t core)
{
	assert(irq >= 0);

	/* TODO NEED to handle migration */
	irq_affinity[irq] = core;
}

uint32_t irq_get_affinity(IRQn_Type irq)
{
	assert(irq >= 0);

	return irq_affinity[irq];
}

void irq_set_priority(IRQn_Type irq, uint32_t hardware_priority)
{
	if (irq < 0 || irq_affinity[irq] == SystemCurrentCore) {
		multicore_irq_set_priority_cmd(irq, hardware_priority);
		return;
	}

	multicore_event_post(SET_PRIORITY_CMD | (hardware_priority << 8) | (irq + 16));

	/* Wait for the value to appear */
	while (irq_get_priority(irq) != hardware_priority);
}

uint32_t irq_get_priority(IRQn_Type irq)
{
	return cls_datum_core(irq_affinity[irq], irq_priority)[irq];
}

void irq_enable(IRQn_Type irq)
{
	/* Are we running on the target core? */
	if (irq < 0 || irq_affinity[irq] == SystemCurrentCore) {
		multicore_irq_enable_cmd(irq);
		return;
	}

	/* Nope, forward to the other core */
	multicore_event_post(IRQ_ENABLE_CMD | (irq + 16));

	/* Wait for it complete */
	while (!irq_is_enabled(irq));
}

void irq_disable(IRQn_Type irq)
{
	/* Are we running on the target core? */
	if (irq < 0 || irq_affinity[irq] == SystemCurrentCore) {
		multicore_irq_disable_cmd(irq);
		return;
	}

	/* Nope, forward to the other core */
	multicore_event_post(IRQ_DISABLE_CMD | (irq + 16));

	/* Wait for it complete */
	while (irq_is_enabled(irq));
}

bool irq_is_enabled(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);

	return irq < 0 || cls_datum_core(irq_affinity[irq], irq_enabled)[irq];
}

void irq_trigger(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);

	/* Are we running on the target core? */
	if (irq < 0 || irq_affinity[irq] == SystemCurrentCore) {
		multicore_irq_pend_cmd(irq);
		return;
	}

	/* Nope, forward to the other core */
	multicore_event_post(PEND_IRQ_CMD | (irq + 16));
}

void irq_clear(IRQn_Type irq)
{
	assert(irq <= __LAST_IRQN);

	/* Is the irq bound to the current core? */
	if (irq < 0 || irq_get_affinity(irq) == SystemCurrentCore) {
		multicore_irq_clear_cmd(irq);
		return;
	}

	/* This is for the other core, send a message */
	multicore_event_post(CLEAR_IRQ_CMD | (irq + 16));
}

static void multicore_irq_init(void)
{
	multicore_event_register(IRQ_EVENT, multicore_irq_event_handler, 0);
}
PREINIT_PLATFORM_WITH_PRIORITY(multicore_irq_init, MULTICORE_IRQ_PLATFORM_INIT_PRIORITY);
