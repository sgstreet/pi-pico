/*
 * Copyright (C) 2017 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * init-sections.c
 *
 * Created on: Mar 16, 2017
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef INIT_SECTIONS_H_
#define INIT_SECTIONS_H_

#include <config.h>
#include <compiler.h>

/* PRIORITY Should always be a 3 digit number */
#define PREINIT_SYSINIT_PRIORITY(PRIORITY) ".preinit_array_sysinit.00" #PRIORITY
#define PREINIT_SYSINIT_SECTION(SECTION) ".preinit_array_sysinit." #SECTION
#define PREINIT_SYSINIT(NAME) void *__attribute__((used, section(PREINIT_SYSINIT_SECTION(NAME)))) preinit_sysinit_##NAME = NAME
#define PREINIT_SYSINIT_WITH_PRIORITY(NAME, PRIORITY) void *__attribute__((used, section(PREINIT_SYSINIT_PRIORITY(PRIORITY)))) preinit_sysinit_##NAME = NAME

#define PREINIT_PLATFORM_PRIORITY(PRIORITY) ".preinit_array_platform.00" #PRIORITY
#define PREINIT_PLATFORM_SECTION(SECTION) ".preinit_array_platform." #SECTION
#define PREINIT_PLATFORM(NAME) void *__attribute__((used, section(PREINIT_PLATFORM_SECTION(NAME)))) preinit_platform_##NAME = NAME
#define PREINIT_PLATFORM_WITH_PRIORITY(NAME, PRIORITY) void *__attribute__((used, section(PREINIT_PLATFORM_PRIORITY(PRIORITY)))) preinit_platform_##NAME = NAME

#define PREINIT_PRIORITY(PRIORITY) ".preinit_array.00" #PRIORITY
#define PREINIT_SECTION(SECTION) ".preinit_array." #SECTION
#define PREINIT(NAME) void *__attribute__((used, section(PREINIT_SECTION(NAME)))) preinit_##NAME = NAME
#define PREINIT_WITH_PRIORITY(NAME, PRIORITY) void *__attribute__((used, section(PREINIT_PRIORITY(PRIORITY)))) preinit_##NAME = NAME

//extern __weak void (*__preinit_array_start[])(void);
//extern __weak void (*__preinit_array_end[])(void);

extern void (*__preinit_array_start[])(void);
extern void (*__preinit_array_end[])(void);

extern __weak void (*__init_array_start[])(void);
extern __weak void (*__init_array_end[])(void);
extern __weak void (*__fini_array_start[])(void);
extern __weak void (*__fini_array_end[])(void);

extern void *__start__;
extern void *__vector;
extern void *__vector_size;
extern void *__inits;
extern void *__inits_size;
extern void *__text;
extern void *__text_size;
extern void *__rodata;
extern void *__rodata_size;
extern void *__unwind_info;
extern void *__unwind_info_size;
extern void *__data;
extern void *__data_start__;
extern void *__data_size;
extern void *__bss;
extern void *__bss_start__;
extern void *__bss_size;
extern void *__heap;
extern void *__heap_size;
extern char __heap_start[];
extern char __heap_end[];
extern void *__stack;

extern void *__tdata;
extern void *__tdata_size;
extern void *__tbss_size;
extern void *__tls_size;
extern void *__tbss_offset;
extern void *__tls_align;
extern void *__arm32_tls_tcb_offset;
extern void *__arm64_tls_tcb_offset;

extern void *__vtor_0;
extern void *__vtor_1;

extern void *__core_0;
extern void *__core_1;

extern void *__stack_0;
extern void *__stack_0_size;

extern void *__stack_1;
extern void *__stack_1_size;

extern void *__fault_stack_0;
extern void *__fault_stack_1;

static inline void run_init_array(void (*start[])(void), void (*end[])(void))
{
	for (int i = 0; i < end - start; i++)
		start[i]();
}

static inline void run_fini_array(void (*start[])(void), void (*end[])(void))
{
	for (int i = end - start; i > 0; i--)
		start[i - 1]();
}

#endif
