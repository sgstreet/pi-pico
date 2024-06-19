/*
 * Copyright (C) 2024 Stephen Street
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tls.c
 *
 *  Created on: Apr 27, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

#include <compiler.h>

#include <sys/tls.h>

#include <init/init-sections.h>
#include <cmsis/cmsis.h>

core_local void *_tls = 0;

__naked void *__aeabi_read_cls(void)
{
	asm volatile (
		".syntax unified\n"
		"ldr r0, =0xd0000000\n"
		"ldr r0, [r0, #0]\n"
		"cmp r0, #0\n"
		"beq 0f\n"
		"ldr r0, =__core_local_1\n"
		"mov pc, lr\n"
		"0: \n"
		"ldr r0, =__core_local_0\n"
		"mov pc, lr\n"
	);}

__naked void *__aeabi_read_core_cls(unsigned long core)
{
	asm volatile (
		".syntax unified\n"
		"cmp r0, #0\n"
		"beq 0f\n"
		"ldr r0, =__core_local_1\n"
		"mov pc, lr\n"
		"0: \n"
		"ldr r0, =__core_local_0\n"
		"mov pc, lr\n"
	);
}

__naked void *__aeabi_read_tp(void)
{
	asm volatile (
		".syntax unified\n"
		"push {r1, lr}\n"
		"ldr r0,=__core_data\n"
		"ldr r1,=_tls\n"
		"subs r1, r1, r0 /* calculate offset of _tls using the core_data block */\n"
		"bl __aeabi_read_cls\n"
		"ldr r0, [r0, r1]\n"
		"pop {r1, pc}\n"
	);
}

void _set_tls(void *tls)
{
	cls_datum(_tls) = tls - ((size_t)&__arm32_tls_tcb_offset);
}

void _init_tls(void *tls)
{
	/* Copy tls initialized data */
	memcpy(tls, __tdata_source, (size_t)__tdata_size);

	/* Clear tls zero data */
	memset(tls + (size_t)__tbss_offset, 0, (size_t)__tbss_size);
}

static void _cls_tls_init(void)
{
	/* Initialize the core local tls pointers */
	for (unsigned long core = 0; core < SystemNumCores; ++core)
		cls_datum_core(core, _tls) = __aeabi_read_core_cls(core) + (size_t)__tls_block_offset - ((size_t)&__arm32_tls_tcb_offset);
}
PREINIT_SYSINIT_WITH_PRIORITY(_cls_tls_init, SYSTEM_INIT_PRIORITY);
