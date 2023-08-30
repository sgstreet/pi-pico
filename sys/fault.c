/*
 * Copyright (C) 2021 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fault.c
 *
 * Created on: Mar 27, 2021
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <compiler.h>
#include <init/init-sections.h>
#include <cmsis/cmsis.h>
#include <sys/fault.h>

extern __weak __noreturn void reset_fault(const struct cortexm_fault *fault);
extern __weak void save_fault(const struct cortexm_fault *fault, const struct backtrace *entries, int count);
extern __weak int _backtrace_unwind(backtrace_t *buffer, int size, backtrace_frame_t *frame);

__weak __noreturn void reset_fault(const struct cortexm_fault *fault)
{
	abort();
}

static __isr_section void assemble_cortexm_fault(struct cortexm_fault *fault, const struct fault_frame *fault_frame, const struct callee_registers *callee_registers)
{
	/* Collect the fault information */
	fault->r0 = fault_frame->r0;
	fault->r1 = fault_frame->r1;
	fault->r2 = fault_frame->r2;
	fault->r3 = fault_frame->r3;
	fault->r4 = callee_registers->r4;
	fault->r5 = callee_registers->r5;
	fault->r6 = callee_registers->r6;
	fault->r7 = callee_registers->r7;
	fault->r8 = callee_registers->r8;
	fault->r9 = callee_registers->r9;
	fault->r10 = callee_registers->r10;
	fault->r11 = callee_registers->r11;

	fault->IP = fault_frame->IP;
	fault->SP = (uint32_t)fault_frame;
	fault->LR = fault_frame->LR;
	fault->PC = fault_frame->PC;
	fault->PSR = fault_frame->PSR;

	fault->fault_type = SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;
}

static __isr_section int backtrace_fault(const struct cortexm_fault *fault, struct backtrace *entries, int size)
{
	struct backtrace_frame backtrace_frame;

	/* Collect backtrace */
	backtrace_frame.fp = fault->r7;
	backtrace_frame.lr = fault->LR;
	backtrace_frame.sp = fault->SP;
	backtrace_frame.pc = fault->PC;

	/* Use PC if it is good */
	/* TODO Make this 100% right not sure what to do on cortex-m0+
	if ((fault->CFSR & (1 << 17)) != 0)
		backtrace_frame.pc = fault->LR;
    */

	return _backtrace_unwind(entries, size, &backtrace_frame);
}

__isr_section void hard_fault(const struct fault_frame *fault_frame, const struct callee_registers *callee_registers, uint32_t exception_return)
{
	struct cortexm_fault fault;
	struct backtrace fault_backtrace[FAULT_BACKTRACE_SIZE];
	int entries = 0;

	/* Assemble the fault information */
	assemble_cortexm_fault(&fault, fault_frame, callee_registers);

	/* Backtrace the fault */
	entries = backtrace_fault(&fault, fault_backtrace, FAULT_BACKTRACE_SIZE);

	__BKPT(100);

	/* Save the fault information */
	save_fault(&fault, fault_backtrace, entries);

	/* Reset the fault */
	reset_fault(&fault);
}

static void fault_init(void)
{
}
PREINIT_SYSINIT_WITH_PRIORITY(fault_init, FAULT_SYSTEM_INIT_PRIORITY);
