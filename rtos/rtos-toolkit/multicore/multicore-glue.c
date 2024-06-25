/*
 * Copyright (C) 2024 Stephen Street
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * multicore-glue.c
 *
 *  Created on: Mar 26, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>

#include <sys/systick.h>
#include <sys/tls.h>
#include <sys/spinlock.h>
#include <sys/async.h>

#include <cmsis/cmsis.h>
#include <rtos/rtos-toolkit/scheduler.h>

#include <hardware/rp2040/multicore-event.h>

extern void scheduler_startup_hook(void);

void scheduler_spin_lock(void);
void scheduler_spin_unlock(void);
unsigned int scheduler_spin_lock_irqsave(void);
void scheduler_spin_unlock_irqrestore(unsigned int state);
void init_fault(void);
void multicore_startup_hook(void);
void multicore_shutdown_hook(void);

static spinlock_t kernel_lock = 0;
struct async multicore_start_async;

void scheduler_spin_lock()
{
	spin_lock(&kernel_lock);
}

void scheduler_spin_unlock(void)
{
	spin_unlock(&kernel_lock);
}

unsigned int scheduler_spin_lock_irqsave(void)
{
	return spin_lock_irqsave(&kernel_lock);
}

void scheduler_spin_unlock_irqrestore(unsigned int state)
{
	spin_unlock_irqrestore(&kernel_lock, state);
}

unsigned long scheduler_num_cores(void)
{
	return SystemNumCores;
}

unsigned long scheduler_current_core(void)
{
	return SystemCurrentCore;
}

void scheduler_request_switch(unsigned long core)
{
	if (core == SystemCurrentCore) {
		SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
		return;
	}

	multicore_event_post(0x30000000 | (PendSV_IRQn + 16));
}

static void multicore_trap(void)
{
	while (true);
}

void init_fault(void)
{
	multicore_event_post((uintptr_t)multicore_trap);
}

static void multicore_start(struct async *async)
{
	/* This will capture the initial frame, we will return here when the scheduler exits, see  */
	scheduler_run();
}

void multicore_startup_hook(void)
{
	/* If we are running on the startup core, launch core 1 */
	if (SystemCurrentCore == 0) {
		async_run(&multicore_start_async, multicore_start, 0);
		return;
	}

	/* Ensure the system tick is running */
	systick_init();
}

void multicore_shutdown_hook(void)
{
	if (SystemCurrentCore != 0)
		SysTick->CTRL = 0;
}
