/*
 * tls.c
 *
 *  Created on: Apr 27, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <cmsis/cmsis.h>

extern void *__arm32_tls_tcb_offset;
extern void *__core_data_size;
extern void *__core_data;
extern char __tdata_source[];
extern char __tdata_size[];
extern char __tbss_size[];
extern char __tbss_offset[];

#undef __fast_section
#define __fast_section

void *_cls = 0;
core_local void *_tls = 0;

void *__aeabi_read_cls(void);
__naked __fast_section void *__aeabi_read_cls(void)
{
	asm volatile (
		".syntax unified\n"
		"push {r1, lr}\n"
		"ldr r0, =0xd0000000\n"
		"ldr r0, [r0, #0]\n"
		"ldr r1, =__core_data_size\n"
		"muls r0, r0, r1\n"
		"ldr r1, =_cls\n"
		"ldr r1, [r1, #0]\n"
		"adds r0, r0, r1\n"
		"pop {r1, pc}\n"
	);
}

void *__aeabi_read_core_cls(unsigned long core);
__naked __fast_section void *__aeabi_read_core_cls(unsigned long core)
{
	asm volatile (
		".syntax unified\n"
		"push {r1, lr}\n"
		"ldr r1, =__core_data_size\n"
		"muls r0, r0, r1\n"
		"ldr r1, =_cls\n"
		"ldr r1, [r1, #0]\n"
		"adds r0, r0, r1\n"
		"pop {r1, pc}\n"
	);
}

void *__wrap___aeabi_read_tp(void);
__naked __fast_section void *__wrap___aeabi_read_tp(void)
{
	asm volatile (
		".syntax unified\n"
		"push {r1, lr}\n"
		"ldr r0,=__core_data\n"
		"ldr r1,=_tls\n"
		"subs r1, r1, r0\n"
		"bl __aeabi_read_cls\n"
		"ldr r0, [r0, r1]\n"
		"pop {r1, pc}\n"
	);
}

void __wrap__set_tls(void *tls);
__fast_section __optimize void __wrap__set_tls(void *tls)
{
	cls_datum(_tls) = tls - ((size_t)&__arm32_tls_tcb_offset);
}

void __wrap__init_tls(void *__tls);
__no_optimize void __wrap__init_tls(void *__tls)
{
	char *src;
	char *dst;
	char *end;

	/* Copy tls initialized data */
	src = __tdata_source;
	dst = __tls;
	end = __tls + (size_t)__tdata_size;
	while (dst < end)
		*src++ = *dst++;

	/* Clear tls zero data */
	dst = __tls + (size_t)__tbss_offset;
	end = __tls + (size_t)__tbss_offset + (size_t)__tbss_size;
	while (dst < end)
		*dst++ = 0;
}

void _init_cls(void *cls_datum);
__no_optimize void _init_cls(void *cls_datum)
{
	char *dst;
	char *src;
	char *end;

	_cls = cls_datum;

	for (size_t i = 0; i < SystemNumCores; ++i) {
		dst = cls_datum + i * (size_t)&__core_data_size;
		src = (char *)&__core_data;
		end = src + (size_t)&__core_data_size;
		while (src < end)
			*dst++ = *src++;
	}
}
