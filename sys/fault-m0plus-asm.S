/*
 * Copyright (C) 2021 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fault-m7-asm.S
 *
 * Created on: Mar 27, 2021
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <asm.h>

declare_function HardFault_Handler, .isr
	.fnstart
	.cantunwind

	/* Pass pointer to exception frame in R0, this is the stack pointer */
	movs       r0, #4
	mov        r1, lr
	tst        r0, r1        /* Test EXEC_RETURN for used stack */
	beq        0f
	mrs        r0, psp       /* Move the process stack poiner to R0 */
	b          1f
0:
	mrs        r0, msp       /* Move the main stack pointer to R0 */
1:

	/* Switch to known good stack based on the current core */
	ldr        r1, =0xd0000000
	ldr        r1, [r1, #0]
	cmp        r1, #0
	beq        2f
	ldr        r1, =__fault_stack_1
	b          3f
2:
	ldr        r1, =__fault_stack_0
3:
	mov        sp, r1

	/* Assemble callee saved registers and pass pointer in R1, need to push callee saved register */
	push       {r4-r7}
	mov        r4, r8
	mov        r5, r9
	mov        r6, r10
	mov        r7, r11
	push       {r4-r7}

	/* Pass the callee saved registers in R1 */
	mov        r1, sp

	/* Pass EXEC_RETURN in r2 */
	mov        r2, lr

	/* Invoke the backend handler, which should never return */
	ldr			r3, =hard_fault
	bx			r3

	.fnend
	.pool
	.size	HardFault_Handler, . - HardFault_Handler
