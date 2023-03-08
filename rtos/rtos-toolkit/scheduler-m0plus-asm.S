/*
 * Copyright (C) 2022 Stephen Street
 *
 * preemt-sched-asm.S
 *
 * Created on: Feb 9, 2017
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <asm.h>

declare_function SVC_Handler_0, .isr
	.fnstart

	/* Get the active stack pointer into r0 we should always be on the msp */
	mov r0, sp

	/* Save the current context */
	mov r2, lr               /* Save the exec return */
	mrs r3, control          /* Rember the control (mostly privilge state) */
	subs r0, r0, #40         /* Reserve space for the remainder of the scheduler frame */
	stmia r0!, {r2-r7}       /* Save the low registers */
	mov r4, r8
	mov r5, r9
	mov r6, r10
	mov r7, r11
	stmia r0!, {r4-r7}       /* Save the hight registers */
	subs r0, r0, #40         /* Make r0 point at the start of the scheduler frame */

	/* Update the stack pointer */
	mov sp, r0

	/* Store the initial frame in the scheduler */
	ldr r1, =scheduler
	ldr r1, [r1]
	str r0, [r1, #4]

	/* Trigger the PendSV */
	ldr r1, =0xe000ed04
	ldr r2, =0x10000000
	str r2, [r1]
	dsb

	/* We should go into the scheduler and only return on scheduler exit */
	bx lr

	.fnend
	.pool
	.size SVC_Handler_0, . - SVC_Handler_0

declare_function PendSV_Handler, .isr
	.fnstart
/*	.cantunwind */

	/* Get the active stack pointer into r0 */
	movs       r0, #4
	mov        r2, lr        /* Save the exec return */
	tst        r0, r2        /* Test EXEC_RETURN for used stack */
	beq        0f
	mrs        r0, psp       /* Move the process stack poiner to R0 */
	b          1f
0:
	mrs        r0, msp       /* Move the main stack pointer to R0 */
1:

	/* Save the current context */
	mrs r3, control          /* Rember the control (mostly privilge state) */
	subs r0, r0, #40         /* Reserve space for the remainder of the scheduler frame */
	stmia r0!, {r2-r7}       /* Save the low registers */
	mov r4, r8
	mov r5, r9
	mov r6, r10
	mov r7, r11
	stmia r0!, {r4-r7}       /* Save the hight registers */
	subs r0, r0, #40         /* Make r0 point at the start of the scheduler frame */

	/* Switch task */
	ldr r1, =scheduler_switch
	blx r1

	/* Load the new context, which was stashed */
	adds r0, r0, #24         /* Move to the start of the high regs */
	ldmia r0!, {r4-r7}       /* Restore the high regs */
	mov r8, r4
	mov r9, r5
	mov r10, r6
	mov r11, r7
	subs r0, r0, #40         /* Move to start of low registers, include the exec return and control */
	ldmia r0!, {r2-r7}       /* Restore them */
	adds r0, #16             /* Skip the high regs, we have already restored them above */

	/* Restore the control register */
	msr control, r3

	/* Set current stack pointer */
	movs r1, #4
	tst r1, r2               /* Check bit 3 of the exec return to determine the stack being used */
	bne 2f
	msr msp, r0              /* Must have be the initial start frame */
	b 3f
2:
	msr psp, r0              /* Must have a thread */
3:

	/* Let it fly */
	isb
	bx r2

	.fnend
	.pool
	.size PendSV_Handler, . - PendSV_Handler

