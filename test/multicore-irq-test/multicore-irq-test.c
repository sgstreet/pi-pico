/*
 * Copyright (C) 2024 Stephen Street
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * multicore-irq-test.c
 *
 *  Created on: Mar 13, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdint.h>
#include <config.h>

#include <sys/irq.h>
#include <sys/systick.h>

#include <board/board.h>

static void blink_led(IRQn_Type irq, void *context)
{
	board_led_toggle((uint32_t)context);
}

int main(int argc, char **argv)
{
	irq_register(SWI_0_IRQn, INTERRUPT_NORMAL, blink_led, 0);
	irq_set_affinity(SWI_0_IRQn, 1);
	irq_enable(SWI_0_IRQn);

	while (true) {
		irq_trigger(SWI_0_IRQn);
		systick_delay(125);
	}

	return 0;
}
